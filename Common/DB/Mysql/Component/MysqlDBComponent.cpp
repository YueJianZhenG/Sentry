//
// Created by zmhy0073 on 2022/7/16.
//

#include "MysqlDBComponent.h"
#include "Entity/Actor/App.h"
#include "Util/File/DirectoryHelper.h"
#include "Log/Component/LoggerComponent.h"
#include "Server/Config/ServerConfig.h"
#include "Server/Component/ThreadComponent.h"
#include "Timer/Timer/ElapsedTimer.h"
#include "Lua/Lib/Lib.h"
#include "XCode/XCode.h"
#include "Server/Config/CodeConfig.h"
#include "DB/Common/SqlFactory.h"
#include "Util/Tools/String.h"

namespace acs
{
	MysqlDBComponent::MysqlDBComponent()
	{
		this->mCount = 0;
		this->mRetryCount = 0;
		sql::RegisterObject();
		db::Explain::RegisterAllFields();
		mysql::Cluster::RegisterAllFields();
		mysql::Explain::RegisterAllFields();
	}

	bool MysqlDBComponent::Awake()
	{
		this->mConfig.count = 1;
		this->mConfig.debug = false;
		LuaCCModuleRegister::Add([](Lua::CCModule& ccModule)
		{
			ccModule.Open("db.mysql", lua::lib::luaopen_lmysqldb);
		});
		return ServerConfig::Inst()->Get("mysql", this->mConfig);
	}

	void MysqlDBComponent::OnStart()
	{
		this->InitDataBaseAndSession();
	}

	bool MysqlDBComponent::InitDataBaseAndSession()
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
						LOG_DEBUG("[{}ms] init mysql table [{}] ok", timer1.GetMs(), tab);
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
			LOG_BY_NAME(sql::LOG_MYSQL, "mysql client ({}) compile sql ok", iter->first)
		}
		return true;
	}

	bool MysqlDBComponent::InitTable(const std::string& name, const sql::Table& tableInfo)
	{
		std::string sql1 = fmt::format("DESCRIBE {}", name);
		std::unique_ptr<mysql::Response> response = this->Run(sql1);
		if (response == nullptr || !response->IsOk() || response->contents.empty())
		{
			if(!this->CreateTable(name, tableInfo))
			{
				return false;
			}
			LOG_BY_NAME(sql::LOG_MYSQL, "create mysql table [{}] ok", name);
			return true;
		}
		sql::Factory sqlFactory;
		std::unordered_map<std::string, std::string> fields;
		for(const std::string & json : response->contents)
		{
			json::r::Document document1;
			if(document1.Decode(json))
			{
				std::string field, type;
				document1.Get("Type", type);
				document1.Get("Field", field);
				fields.emplace(field, help::Str::Toupper(type));
			}
		}

		for(auto iter = tableInfo.fields.begin(); iter != tableInfo.fields.end(); iter++)
		{
			const std::string & field = iter->first;
			if(fields.find(field) == fields.end())
			{
				sqlFactory.GetTable(name);
				const sql::Field & fieldInfo = iter->second;
				sqlFactory.AlterColumn(field.c_str(), fieldInfo.type);
				if(fieldInfo.notnull)
				{
					sqlFactory.NotNull();
				}
				sqlFactory.Default(fieldInfo.default_val);
				const std::string & sql = sqlFactory.ToString();
				std::unique_ptr<mysql::Response> mysqlResponse = this->Run(sql);
				if(mysqlResponse != nullptr && mysqlResponse->IsOk())
				{
					LOG_INFO("[{}] add column ({}) ok", name, field);
				}
				if(fieldInfo.index)
				{
					sqlFactory.GetTable(name).SetIndex(field, false);
					const std::string & sql2 = sqlFactory.ToString();
					std::unique_ptr<mysql::Response> mysqlResponse1 = this->Run(sql2);
					if(mysqlResponse1 != nullptr && mysqlResponse1->IsOk())
					{
						LOG_DEBUG("[{}] set index ({}) ok", name, field);
					}
				}
			}
		}
		return true;
	}

	bool MysqlDBComponent::CreateTable(const std::string& name, const sql::Table& tableInfo)
	{
		json::r::Value jsonFields;
		std::string engine("InnoDB");
		std::string character("utf8mb4");
		std::string collate("utf8mb4_unicode_ci");

		size_t count = 0;
		sql::Factory sqlFactory;
		std::vector<std::string> indexes;
		std::vector<std::string> primary;
		sqlFactory.GetTable(name).Begin();
		for (auto iter = tableInfo.fields.begin(); iter != tableInfo.fields.end(); iter++)
		{
			count++;
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
			if(fieldInfo.auto_inc)
			{
				sqlFactory.AutoInc();
			}
			if(!fieldInfo.comment.empty())
			{
				sqlFactory.Comment(fieldInfo.comment);
			}
			if (fieldInfo.index)
			{
				indexes.emplace_back(fieldInfo.name);
			}
			if (fieldInfo.primary_key)
			{
				primary.emplace_back(fieldInfo.name);
			}
			if(count < tableInfo.fields.size())
			{
				sqlFactory.Next();
			}
		}
		if(!primary.empty())
		{
			sqlFactory.Next().PrimaryKey(primary);
		}
		if(!indexes.empty())
		{
			sqlFactory.Next().Index(indexes);
		}
		sqlFactory.End().Engine(engine.c_str()).Character(character.c_str()).Collate(collate.c_str());
		std::unique_ptr<mysql::Response> response = this->Run(sqlFactory.ToString());
		return response != nullptr && response->IsOk();
	}

	bool MysqlDBComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(!this->mConfig.address.empty())
		for (int x = 0; x < this->mConfig.address.size(); x++)
		{
			const std::string& address = this->mConfig.address.at(x);
			for (int index = 0; index < this->mConfig.count; index++)
			{
				mysql::Config config;
				timer::ElapsedTimer timer1;
				if (!MysqlDBComponent::DecodeUrl(address, config))
				{
					return false;
				}
				this->mRetryCount++;
				int id = (x + 1) * 100 + index + 1;
				config.script = this->mConfig.table;
				config.conn_count = this->mConfig.conn_count;
				Asio::Context& main = this->mApp->GetContext();
				tcp::Socket* tcpSocket = this->GetComponent<ThreadComponent>()->CreateSocket(config.address);
				std::shared_ptr<mysql::Client> mysqlClient = std::make_shared<mysql::Client>(id, this, config, main);
				{
					int code = mysqlClient->Start(tcpSocket);
					if (code != XCode::Ok)
					{
						LOG_ERROR("connect {} {} ", address, CodeConfig::Inst()->GetDesc(code));
						return false;
					}
					this->mFreeClients.Push(id);
					this->mClients.emplace(id, mysqlClient);
					const mysql::HandshakeResponse & handShake = mysqlClient->GetHandshake();
					LOG_INFO("[{}ms] ({}) connect {} ok ", timer1.GetMs(), handShake.server_version, address);
				}
			}
		}
		return true;
	}

	bool MysqlDBComponent::DecodeUrl(const std::string& address, mysql::Config& config)
	{
		if (!config.Decode(address))
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

	void MysqlDBComponent::OnRecord(json::w::Document& document)
	{
		timer::ElapsedTimer timer1;
		std::unique_ptr<mysql::Request> pingRequest
				= std::make_unique<mysql::Request>(mysql::cmd::PING);

		std::unique_ptr<mysql::Response> response = this->Run(pingRequest);
		std::unique_ptr<json::w::Value> jsonObject = document.AddObject("mysql");
		{
			if(response->HasError() && !response->error.empty())
			{
				jsonObject->Add("error", response->error.front());
			}

			jsonObject->Add("sum", this->mCount);
			jsonObject->Add("retry", this->mRetryCount);
			jsonObject->Add("client", this->mClients.size());
			jsonObject->Add("free", this->mFreeClients.Size());
			jsonObject->Add("wait", this->mMessages.size());
			jsonObject->Add("ping", fmt::format("{}ms", timer1.GetMs()));
		}
	}

	void MysqlDBComponent::OnConnectOK(int id)
	{
		this->mRetryCount--;
		if (this->mClients.find(id) != this->mClients.end())
		{
			this->AddFreeClient(id);
			LOG_DEBUG("mysql client:{} login ok", id);
		}
	}

	void MysqlDBComponent::OnDestroy()
	{
		while (!this->mMessages.empty())
		{
			this->mApp->Sleep();
			LOG_DEBUG("wait mysql request invoke => {}", this->mMessages.size());
		}
	}

	void MysqlDBComponent::OnMessage(int id, mysql::Request* req, mysql::Response* res) noexcept
	{
		this->mCount++;
		this->AddFreeClient(id);
		std::unique_ptr<mysql::Request> request(req);
		std::unique_ptr<mysql::Response> response(res);
		if (response->HasError())
		{
			size_t index = 0;
			LOG_WARN("{}", request->ToString());
			for(const std::string & error : response->error)
			{
				LOG_ERROR("[{}] => {}", ++index, error);
			}
		}
#ifdef __DEBUG__
		else
		{
			if (this->mConfig.explain.open && request->Count() == 1)
			{
				std::string cmd;
				if (request->GetCommand(cmd) && cmd != "EXPLAIN" && this->mConfig.explain.HasCommand(cmd))
				{
					std::string sql = request->ToString();
					long long ms = request->GetCostTime();
					CoroutineComponent * coroutine = acs::App::Coroutine();
					coroutine->Start(&MysqlDBComponent::OnExplain, this, sql, ms);
				}
			}
			if (this->mConfig.debug)
			{
				CONSOLE_LOG_DEBUG("request=>{}", request->ToString());
				CONSOLE_LOG_DEBUG("[{}ms] response=>{}", request->GetCostTime(), response->ToString());
			}
		}
#endif
		int rpcId = request->GetRpcId();
		if (rpcId > 0)
		{
			this->OnResponse(rpcId, response);
		}
	}

	void MysqlDBComponent::OnExplain(const std::string& sql, long long ms) noexcept
	{
		const std::string newSql = fmt::format("EXPLAIN {}", sql);
		std::unique_ptr<mysql::Response> response1 = this->Run(newSql);
		LoggerComponent* logger = this->GetComponent<LoggerComponent>();
		{
			if(!response1->contents.empty())
			{
				std::unique_ptr<custom::LogInfo> sqlLog = std::make_unique<custom::LogInfo>();
				{
					sqlLog->Level = custom::LogLevel::None;
					sqlLog->Content = fmt::format("[{}ms] {}\n", ms, sql);
					{
						sqlLog->Content = response1->contents.front();
						logger->PushLog("mysql", sqlLog);
					}
				}
			}
		}
	}

	void MysqlDBComponent::OnSendFailure(int id, mysql::Request* message)
	{
		// 重新找一个mysql client执行
		std::unique_ptr<mysql::Request> request(message);
		LOG_WARN("redis client:{} resend => {}", id, request->ToString())
		this->Send(request);
	}

	void MysqlDBComponent::OnClientError(int id, int code)
	{
		auto iter = this->mClients.find(id);
		if (iter == this->mClients.end())
		{
			return;
		}
		this->mRetryCount++;
		this->mRetryClients.emplace(id);
		std::string address = iter->second->GetAddress();
		LOG_WARN("mysql client ({}) login fail", address)
	}

	void MysqlDBComponent::OnSecondUpdate(int tick) noexcept
	{
		if (this->mConfig.ping > 0 && tick % this->mConfig.ping == 0)
		{
			int id = 0;
			while (this->mFreeClients.Pop(id))
			{
				std::unique_ptr<mysql::Request> request =
					std::make_unique<mysql::Request>(mysql::cmd::PING);
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


	void MysqlDBComponent::Send(std::unique_ptr<mysql::Request>& request, int& rpcId)
	{
		rpcId = this->BuildRpcId();
		{
			request->SetRpcId(rpcId);
			this->Send(request);
		}
	}

	void MysqlDBComponent::AddFreeClient(int id)
	{
		if (!this->mMessages.empty())
		{
			this->Send(id, this->mMessages.front());
			this->mMessages.pop();
			return;
		}
		this->mFreeClients.Push(id);
	}

	void MysqlDBComponent::Send(int id, std::unique_ptr<mysql::Request>& request)
	{
		auto iter = this->mClients.find(id);
		if (iter == this->mClients.end())
		{
			return;
		}
		iter->second->Send(std::move(request));
	}

	void MysqlDBComponent::Send(std::unique_ptr<mysql::Request>& request)
	{
		int id = 0;
		if (!this->mFreeClients.Pop(id))
		{
			this->mMessages.emplace(std::move(request));
			return;
		}
		this->Send(id, request);
	}

	std::unique_ptr<mysql::Response> MysqlDBComponent::Run(const std::string& sql)
	{
		std::unique_ptr<mysql::Request> request =
				std::make_unique<mysql::Request>(sql);
		return this->Run(request);
	}

	std::unique_ptr<json::r::Document> MysqlDBComponent::Invoke(const std::string& sql)
	{
		std::unique_ptr<mysql::Response> response = this->Run(sql);
		if (response == nullptr || !response->IsOk() || response->contents.empty())
		{
			return nullptr;
		}
		const std::string & result = response->contents.front();
		std::unique_ptr<json::r::Document> document = std::make_unique<json::r::Document>();
		{
			if (!document->Decode(result.c_str(), result.size()))
			{
				return nullptr;
			}
		}
		return document;
	}

	std::unique_ptr<mysql::Response> MysqlDBComponent::Run(std::unique_ptr<mysql::Request>& request)
	{
		int rpcId = this->BuildRpcId();
		unsigned int timeout = this->mConfig.timeout;
		{
			request->SetRpcId(rpcId);
			this->Send(request);
			return this->BuildRpcTask<MysqlTask>(rpcId, timeout)->Await();
		}
	}
}