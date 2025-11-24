//
// Created by 64658 on 2025/2/18.
//

#include "XCode/XCode.h"
#include "Lua/Lib/Lib.h"
#include "Entity/Actor/App.h"
#include "PgsqlDBComponent.h"

#include "Server/Config/CodeConfig.h"
#include "Server/Config/ServerConfig.h"
#include "Util/File/DirectoryHelper.h"
#include "Server/Component/ThreadComponent.h"

#include "Timer/Timer/ElapsedTimer.h"
#include "Log/Component/LoggerComponent.h"
#include "DB/Common/SqlFactory.h"
#include "Util/Tools/String.h"
namespace acs
{
	PgsqlDBComponent::PgsqlDBComponent()
	{
		this->mSumCount = 0;
		this->mRetryCount = 0;
		this->mConfig.count = 1;
		this->mConfig.debug = false;
		sql::RegisterObject();
		db::Explain::RegisterAllFields();
		pgsql::Cluster::RegisterAllFields();
	}

	bool PgsqlDBComponent::Awake()
	{
		LuaCCModuleRegister::Add([](Lua::CCModule& ccModule) {
			ccModule.Open("db.pgsql", lua::lib::luaopen_lpgsqldb);
		});
		LOG_CHECK_RET_FALSE(ServerConfig::Inst()->Get("pgsql", this->mConfig))
		return true;
	}

	bool PgsqlDBComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(!this->mConfig.address.empty())
		for (int x = 0; x < this->mConfig.address.size(); x++)
		{
			const std::string& address = this->mConfig.address[x];
			for (int index = 0; index < this->mConfig.count; index++)
			{
				pgsql::Config config;
				timer::ElapsedTimer timer1;
				if (!PgsqlDBComponent::DecodeUrl(address, config))
				{
					return false;
				}
				this->mRetryCount++;
				int id = (x + 1) * 100 + index + 1;
				config.table = this->mConfig.table;
				config.conn_count = this->mConfig.conn_count;
				Asio::Context& main = this->mApp->GetContext();
				tcp::Socket* tcpSocket = this->GetComponent<ThreadComponent>()->CreateSocket(config.address);
				std::shared_ptr<pgsql::Client> client = std::make_shared<pgsql::Client>(id, this, config, main);
				{
					int code = client->Start(tcpSocket);
					if (code != XCode::Ok)
					{
						std::string desc = CodeConfig::Inst()->GetDesc(code);
						LOG_ERROR("connect {} => {} ", address, desc);
						return false;
					}
					this->mClients.emplace(id, client);
					const pgsql::ServerInfo & info = client->GetInfo();
					LOG_INFO("[{}ms] ({}) connect {} ok", timer1.GetMs(), info.serverVersion, address);
				}
			}
		}
		return true;
	}

	void PgsqlDBComponent::OnStart()
	{
		this->InitDataBase();
	}

	bool PgsqlDBComponent::InitDataBase()
	{
		std::vector<std::string> filePaths;
		const std::string& dir = this->mConfig.table;
		if (help::dir::GetFilePaths(dir, ".json", filePaths) > 0)
		{
			for (const std::string& path: filePaths)
			{
				if(!this->mFileFactory.IsChange(path))
				{
					continue;
				}
				sql::Table mysqlTable;
				json::r::Document document;
				if (!this->mFileFactory.Read(path, document, mysqlTable))
				{
					LOG_ERROR("decode json fail => {}", path);
					return false;
				}

				std::vector<std::string> tables;
				tables.emplace_back(mysqlTable.name);
				if(mysqlTable.count > 0)
				{
					tables.clear();
					for (int index = 0; index < mysqlTable.count; index++)
					{
						int idx = index + 1;
						tables.emplace_back(fmt::format("{}{}", mysqlTable.name, idx));
					}
				}
				for (const std::string& tab: tables)
				{
					timer::ElapsedTimer timer1;
					if (this->InitTable(tab, mysqlTable))
					{
						LOG_DEBUG("[{}ms] init pgsql table [{}] ok", timer1.GetMs(), tab);
					}
				}
			}
		}
		assert(this->mFreeClients.Size() == this->mClients.size());
		for(auto iter = this->mClients.begin(); iter != this->mClients.end(); iter++)
		{
			if(!iter->second->InvokeCompileSql())
			{
				return false;
			}
		}
		return true;
	}

	bool PgsqlDBComponent::InitTable(const std::string& name, const sql::Table& tableInfo)
	{
		std::string table = name;
		if(!tableInfo.schema.empty())
		{
			const std::string &schema = tableInfo.schema;
			std::string sql = fmt::format("CREATE SCHEMA IF NOT EXISTS {}", schema);
			std::unique_ptr<pgsql::Response> response1 = this->Run(sql);
			if(response1 == nullptr || !response1->IsOk())
			{
				LOG_ERROR("[pgsql] create schema:{} fail", schema)
				return false;
			}
			table = fmt::format("{}.{}", schema, name);
		}
		sql::Factory sqlFactory;
		sqlFactory.GetTable("information_schema.columns");
		if(!tableInfo.schema.empty())
		{
			const std::string &schema = tableInfo.schema;
			std::string filter = fmt::format("table_schema='{}' AND table_name='{}'", schema, name);
			sqlFactory.Select({ "column_name", "data_type"}).Filter(filter);
		}
		else
		{
			std::string filter = fmt::format("table_name='{}'", name);
			sqlFactory.Select({ "column_name", "data_type"}).Filter(filter);
		}

		std::unique_ptr<pgsql::Response> response = this->Run(sqlFactory.ToString());
		if (response == nullptr || !response->IsOk() || response->results.empty())
		{
			if(!this->CreateTable(table, tableInfo))
			{
				return false;
			}
			LOG_INFO("create mysql table [{}] ok", table);
			return true;
		}
		std::unordered_map<std::string, std::string> fields;
		for(const std::string & json : response->results)
		{
			json::r::Document document1;
			if(document1.Decode(json))
			{
				std::string field, type;
				document1.Get("data_type", type);
				document1.Get("column_name", field);
				fields.emplace(field, help::Str::Toupper(type));
			}
		}

		for(auto iter = tableInfo.fields.begin(); iter != tableInfo.fields.end(); iter++)
		{
			const std::string & field = iter->first;
			if(fields.find(field) == fields.end())
			{
				sqlFactory.GetTable(table);
				const sql::Field & fieldInfo = iter->second;
				sqlFactory.AlterColumn(field.c_str(), fieldInfo.type);
				if(fieldInfo.notnull)
				{
					sqlFactory.NotNull();
				}
				sqlFactory.Default(fieldInfo.default_val);
				const std::string sql = sqlFactory.ToString();
				std::unique_ptr<pgsql::Response> pgsqlResponse = this->Run(sql);
				if(pgsqlResponse != nullptr && pgsqlResponse->IsOk())
				{
					LOG_DEBUG("[{}] add column ({}) ok", table, field);
				}
				if(fieldInfo.index)
				{
					sqlFactory.GetTable(table).SetIndex(field, false);
					const std::string sql2 = sqlFactory.ToString();
					std::unique_ptr<pgsql::Response> mysqlResponse1 = this->Run(sql2);
					if(mysqlResponse1 != nullptr && mysqlResponse1->IsOk())
					{
						LOG_DEBUG("[{}] set index ({}) ok", table, field);
					}
				}
			}
		}
		return true;
	}

	bool PgsqlDBComponent::CreateTable(const std::string& name, const sql::Table& tableInfo)
	{
		sql::Factory sqlFactory;
		json::r::Value jsonFields;
		std::vector<std::string> indexes;
		std::vector<std::string> primary;
		sqlFactory.GetTable(name).Begin();
		for (auto iter = tableInfo.fields.begin(); iter != tableInfo.fields.end(); iter++)
		{
			const sql::Field& fieldInfo = iter->second;
			sqlFactory.AppendColumn(fieldInfo.name.c_str(), fieldInfo.type.c_str());
			if (fieldInfo.notnull)
			{
				sqlFactory.NotNull();
			}
			sqlFactory.Default(fieldInfo.default_val);
			if (fieldInfo.unique)
			{
				sqlFactory.Unique();
			}
			if (fieldInfo.index)
			{
				indexes.emplace_back(fieldInfo.name);
			}
			if (fieldInfo.primary_key)
			{
				primary.emplace_back(fieldInfo.name);
			}
			sqlFactory.Next();
		}
		sqlFactory.PrimaryKey(primary).End();
		const std::string sql = sqlFactory.ToString();
		std::unique_ptr<pgsql::Response> response = this->Run(sql);
		if(response == nullptr || !response->IsOk())
		{
			return false;
		}
		LOG_INFO("create pgsql [{}] ok", name);
		for(const std::string & field : indexes)
		{
			sqlFactory.GetTable(name).SetIndex(field, false);
			std::unique_ptr<pgsql::Response> response = this->Run(sqlFactory.ToString());
			if(response != nullptr && response->IsOk())
			{
				LOG_INFO("[{}] create pgsql index [{}] ok", name, field);
			}
		}
		return true;
	}

	void PgsqlDBComponent::OnDestroy()
	{
		while(!this->mMessages.empty())
		{
			this->mApp->Sleep();
			LOG_DEBUG("wait pgsql request invoke => {}", this->mMessages.size());
		}
	}

	bool PgsqlDBComponent::DecodeUrl(const std::string& url, pgsql::Config& config)
	{
		if (!config.Decode(url))
		{
			return false;
		}
		std::string ip, port;
		config.Get("db", config.db);
		config.Get("password", config.password);
		LOG_CHECK_RET_FALSE(config.Get("user", config.user))
		LOG_CHECK_RET_FALSE(config.Get("address", config.address))
		return true;
	}

	void PgsqlDBComponent::OnConnectOK(int id)
	{
		this->mRetryCount--;
		if (this->mClients.find(id) != this->mClients.end())
		{
			this->AddFreeClient(id);
			LOG_DEBUG("pgsql client:{} login ok", id);
		}
	}

	void PgsqlDBComponent::OnClientError(int id, int code)
	{
		auto iter = this->mClients.find(id);
		if (iter == this->mClients.end())
		{
			return;
		}
		this->mRetryCount++;
		this->mRetryClients.emplace(id);
		std::string address = iter->second->GetAddress();
		LOG_WARN("pgsql client ({}) login fail", address)
	}

	void PgsqlDBComponent::OnSecondUpdate(int tick) noexcept
	{
		if (this->mConfig.ping > 0 && tick % this->mConfig.ping == 0)
		{
			int id = 0;
			std::string sql("SELECT 1");
			while (this->mFreeClients.Pop(id))
			{
				std::unique_ptr<pgsql::Request> request
					= std::make_unique<pgsql::Request>(sql);
				this->Send(id, request);
			}
		}
		if (this->mConfig.retry > 0 && tick % this->mConfig.retry == 0)
		{
			for (const int id: this->mRetryClients)
			{
				auto iter = this->mClients.find(id);
				if (iter != this->mClients.end())
				{
					iter->second->Start(nullptr);
				}
			}
			this->mRetryClients.clear();
		}
	}

	void PgsqlDBComponent::OnRecord(json::w::Document& document)
	{
		timer::ElapsedTimer timer1;
		std::unique_ptr<pgsql::Response> response = this->Run("SELECT 1");
		std::unique_ptr<json::w::Value> jsonValue = document.AddObject("pgsql");
		{
			if(!response->error.empty())
			{
				jsonValue->Add("error", response->error.front());
			}
			jsonValue->Add("sum", this->mSumCount);
			jsonValue->Add("retry", this->mRetryCount);
			jsonValue->Add("client", this->mClients.size());
			jsonValue->Add("free", this->mFreeClients.Size());
			jsonValue->Add("ping", fmt::format("{}ms", timer1.GetMs()));
			jsonValue->Add("wait", this->mMessages.size() + this->AwaitCount());
		}
	}

	void PgsqlDBComponent::OnSendFailure(int id, pgsql::Request* message)
	{
		std::unique_ptr<pgsql::Request> request(message);
		LOG_WARN("[{}] resend pgsql => {}", id, request->ToString())
		{
			this->Send(request);
		}
	}

	void PgsqlDBComponent::OnExplain(const std::string& sql, long long ms) noexcept
	{
		const std::string newSql = fmt::format("EXPLAIN {}", sql);
		std::unique_ptr<pgsql::Response> response1 = this->Run(newSql);
		LoggerComponent* logger = this->GetComponent<LoggerComponent>();
		{
			if (!response1->results.empty())
			{
				const std::string& result = response1->results.front();
				std::unique_ptr<custom::LogInfo> sqlLog = std::make_unique<custom::LogInfo>();
				{
					sqlLog->Level = custom::LogLevel::None;
					sqlLog->Content = fmt::format("[{}ms] {}\n", ms, sql);
					{
						sqlLog->Content.append(result);
						logger->PushLog("pgsql", sqlLog);
					}
				}
			}
		}
	}

	void PgsqlDBComponent::OnMessage(int id, pgsql::Request* req, pgsql::Response* res) noexcept
	{
		this->mSumCount++;
		this->AddFreeClient(id);
		std::unique_ptr<pgsql::Request> request(req);
		std::unique_ptr<pgsql::Response> response(res);

		int rpcId = request->GetRpcId();
		if (!response->error.empty())
		{
			LOG_WARN("[request] {}", request->ToString());
			for(const std::string & error : response->error)
			{
				LOG_ERROR("[response] {}", error);
			}
		}
#ifdef __DEBUG__
		else
		{
			if (this->mConfig.explain.open && rpcId > 0)
			{
				std::string cmd;
				if (request->GetCommand(cmd) && cmd != "EXPLAIN" && this->mConfig.explain.HasCommand(cmd))
				{
					long long ms = request->GetCostTime();
					std::string sql = request->ToString();
					CoroutineComponent * coroutine = acs::App::Coroutine();
					coroutine->Start(&PgsqlDBComponent::OnExplain, this, sql, ms);
				}
			}

			if (this->mConfig.debug)
			{
				LOG_DEBUG("[{}ms] ({}) {}", request->GetCostTime(), response->results.size(), request->ToString())
			}
		}
#endif
		if (rpcId > 0)
		{
			this->OnResponse(rpcId, response);
		}
	}

	std::unique_ptr<pgsql::Response> PgsqlDBComponent::Run(const std::string& sql)
	{
		std::unique_ptr<pgsql::Request> request
			= std::make_unique<pgsql::Request>(sql);
		return this->Run(request);
	}

	std::unique_ptr<pgsql::Response> PgsqlDBComponent::Run(std::unique_ptr<pgsql::Request>& request)
	{
		int rpcId = 0;
		this->Send(request, rpcId);
		return this->BuildRpcTask<PgsqlTask>(rpcId)->Await();
	}

	void PgsqlDBComponent::Send(std::unique_ptr<pgsql::Request>& request)
	{
		int rpcId = 0;
		this->Send(request, rpcId);
	}

	void PgsqlDBComponent::AddFreeClient(int id)
	{
		if(!this->mMessages.empty())
		{
			this->Send(id, this->mMessages.front());
			this->mMessages.pop();
			return;
		}
		this->mFreeClients.Push(id);
	}

	void PgsqlDBComponent::Send(std::unique_ptr<pgsql::Request>& request, int& rpcId)
	{
		int id = 0;
		rpcId = this->BuildRpcId();
		request->SetRpcId(rpcId);
		if (!this->mFreeClients.Pop(id))
		{
			this->mMessages.emplace(std::move(request));
			return;
		}
		this->Send(id, request);
	}

	void PgsqlDBComponent::Send(int id, std::unique_ptr<pgsql::Request>& request)
	{
		auto iter = this->mClients.find(id);
		if (iter != this->mClients.end())
		{
			iter->second->Send(request);
			return;
		}
		LOG_ERROR("not find client:{} {}", id, request->ToString())
	}
}