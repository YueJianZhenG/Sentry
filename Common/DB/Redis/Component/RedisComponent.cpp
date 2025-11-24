#include "RedisComponent.h"
#include "Lua/Lib/Lib.h"
#include "Entity/Actor/App.h"
#include "Util/File/FileHelper.h"
#include "Util/File/DirectoryHelper.h"
#include "Timer/Timer/ElapsedTimer.h"
#include "Server/Component/ThreadComponent.h"


namespace acs
{
	RedisComponent::RedisComponent()
	{
		this->mSumCount = 0;
		this->mRetryCount = 0;
		redis::Cluster::RegisterAllFields();
	}

    bool RedisComponent::Awake()
    {
		LuaCCModuleRegister::Add([](Lua::CCModule & ccModule) {
			ccModule.Open("db.redis", lua::lib::luaopen_lredisdb);
		});
		return ServerConfig::Inst()->Get("redis", this->mConfig);
    }

	void RedisComponent::OnStart()
	{
		if(!this->LoadRedisScript(this->mConfig.script))
		{
			LOG_ERROR("load lua script fail => {}", this->mConfig.script)
		}
	}

	void RedisComponent::OnRecord(json::w::Document& document)
	{
		timer::ElapsedTimer timer1;
		auto response = this->Run("PING");
		std::unique_ptr<json::w::Value> data = document.AddObject("redis");
		{
			if(response->HasError())
			{
				data->Add("error", response->element.message);
			}
			data->Add("sum", this->mSumCount);
			data->Add("retry", this->mRetryCount);
			data->Add("client", this->mClients.size());
			data->Add("free", this->mFreeClients.Size());
			data->Add("ping", fmt::format("{}ms", timer1.GetMs()));
			data->Add("wait", this->AwaitCount() + this->mRequests.size());
		}
	}

    bool RedisComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(!this->mConfig.address.empty())
		for (int x = 0; x < this->mConfig.address.size(); x++)
		{
			const std::string & address = this->mConfig.address[x];
			for (int index = 0; index < this->mConfig.count; index++)
			{
				redis::Config config;
				if (!RedisComponent::DecodeUrl(address, config))
				{
					return false;
				}
				this->mRetryCount++;
				timer::ElapsedTimer timer1;
				int id = (x + 1) * 100 + index + 1;
				config.conn_count = this->mConfig.conn_count;
				Asio::Context& io = this->mApp->GetContext();
				tcp::Socket* socket = this->GetComponent<ThreadComponent>()->CreateSocket(address);
				std::shared_ptr<redis::Client> redisClient = std::make_shared<redis::Client>(id, config, this, io);
				{
					if (!redisClient->Start(socket))
					{
						LOG_ERROR("connect {} fail", address);
						return false;
					}
					this->mClients.emplace(id, redisClient);
					const std::string & version = redisClient->GetVersion();
					LOG_INFO("[{}ms] ({}) connect {} ok", timer1.GetMs(), version, address);
				}
			}
		}
		return true;
	}

	bool RedisComponent::DecodeUrl(const std::string& url, redis::Config& config)
	{
		if(!config.Decode(url))
		{
			return false;
		}
		config.Get("db", config.db);
		config.Get("password", config.password);
		LOG_CHECK_RET_FALSE(config.Get("address", config.address))
		return true;
	}

	void RedisComponent::OnConnectOK(int id)
	{
		if(this->mClients.find(id) != this->mClients.end())
		{
			this->mRetryCount--;
			this->AddFreeClient(id);
			LOG_DEBUG("redis client:{} login ok", id);
		}
	}

	void RedisComponent::OnClientError(int id, int code)
	{
		auto iter = this->mClients.find(id);
		if (iter == this->mClients.end())
		{
			return;
		}
		this->mRetryCount++;
		this->mRetryClients.emplace(id);
		std::string address = iter->second->GetAddress();
		LOG_WARN("redis client ({}) login fail", address)
	}

	void RedisComponent::OnSendFailure(int id, redis::Request* message)
	{
		std::unique_ptr<redis::Request> request(message);
		LOG_WARN("redis client:{} resend => {}", id, request->ToString())
		this->Send(request);
	}

	void RedisComponent::OnSecondUpdate(int tick) noexcept
	{
		if(this->mConfig.ping > 0 && (tick % this->mConfig.ping) == 0)
		{
			int id = 0;
			while(this->mFreeClients.Pop(id))
			{
				std::unique_ptr<redis::Request> request = redis::Request::Make("PING");
				{
					request->SetRpcId(0);
					this->Send(id, request);
					//CONSOLE_LOG_WARN("redis client:{} ping", id)
				}
			}
		}
		if(this->mConfig.retry > 0 && (tick % this->mConfig.retry) == 0) //重试
		{
			for(const int id : this->mRetryClients)
			{
				auto iter = this->mClients.find(id);
				if(iter != this->mClients.end())
				{
					iter->second->Start(nullptr);
				}
				LOG_WARN("redis client:{} try login", id);
			}
			this->mRetryClients.clear();
		}
	}

	void RedisComponent::OnMessage(int id, redis::Request * req, redis::Response * res) noexcept
	{
		this->mSumCount++;
		this->AddFreeClient(id);
		std::unique_ptr<redis::Request> request(req);
		std::unique_ptr<redis::Response> response(res);
		if (response->HasError())
		{
			LOG_ERROR("redis request = {}", request->ToString());
			LOG_ERROR("redis response = {}", response->ToString());
		}
#ifdef __DEBUG__
		else if(this->mConfig.debug)
		{
			CONSOLE_LOG_DEBUG("[request] = {}", request->ToString());
			CONSOLE_LOG_DEBUG("[response:{}ms] = {}", request->GetCostTime(), response->ToString());
		}
#endif
		int rpcId = request->GetRpcId();
		if(rpcId > 0)
		{
			this->OnResponse(rpcId, response);
		}
	}

    bool RedisComponent::Ping()
    {
        std::unique_ptr<redis::Request> request = redis::Request::Make("PING");
		std::unique_ptr<redis::Response> response = this->Run(request);
        return response != nullptr && !response->HasError();
    }

	std::unique_ptr<redis::Response> RedisComponent::Run(std::unique_ptr<redis::Request> & request) noexcept
    {
		int taskId = 0;
		this->Send(request, taskId);
		return this->BuildRpcTask<RedisTask>(taskId, this->mConfig.timeout)->Await();
    }

	void RedisComponent::Send(std::unique_ptr<redis::Request> & request)
	{
		int id = 0;
		if(this->mFreeClients.Pop(id))
		{
			this->Send(id, request);
			return;
		}
		this->mRequests.push(std::move(request));
	}

	bool RedisComponent::LoadRedisScript(const std::string & dir)
	{
		if(dir.empty()) return true;
		std::vector<std::string> luaFiles;
		help::dir::GetFilePaths(dir, "*.lua", luaFiles);
		for (const std::string& path : luaFiles)
		{
			std::string content, name;
			if (!help::fs::ReadTxtFile(path, content))
			{
				LOG_ERROR("load lua {} fail", path);
				return false;
			}
			if (!help::fs::GetFileName(path, name))
			{
				return false;
			}
			timer::ElapsedTimer timer1;
			std::unique_ptr<redis::Response> response = this->Run("SCRIPT", "LOAD", content);
			{
				LOG_CHECK_RET_FALSE(response != nullptr && response->element.IsString());
				if(!this->OnLoadScript(name, response->element.message))
				{
					LOG_ERROR("load redis script [{}.lua] fail", name);
					return false;
				}
				LOG_DEBUG("[{}ms] load redis script [{}.lua] ok", timer1.GetMs(), name);
			}
		}
		return true;
	}

	bool RedisComponent::OnLoadScript(const std::string& name, const std::string& md5)
	{
		auto iter = this->mLuaMap.find(name);
		if(iter != this->mLuaMap.end())
		{
			return false;
		}
		this->mLuaMap.emplace(name, md5);
		return true;
	}

	bool RedisComponent::Send(const acs::RedisLuaData& data) noexcept
	{
		std::unique_ptr<redis::Request> request;
		if (!this->MakeLuaRequest(data, request))
		{
			return false;
		}

		this->Send(request);
		return true;
	}

	bool RedisComponent::Send(const acs::RedisLuaData& data, int& taskId, unsigned int & timeout) noexcept
	{
		std::unique_ptr<redis::Request> request;
		if (!this->MakeLuaRequest(data, request))
		{
			return false;
		}
		this->Send(request, taskId);
		timeout = this->mConfig.timeout;
		return true;
	}

	std::unique_ptr<redis::Response> RedisComponent::CallLua(const acs::RedisLuaData& data)
	{
		std::unique_ptr<redis::Request> request;
		if(!this->MakeLuaRequest(data, request))
		{
			return nullptr;
		}
		return this->Run(request);
	}

	std::unique_ptr<json::r::Document> RedisComponent::Call(const acs::RedisLuaData& data)
	{
		std::unique_ptr<redis::Request> request;
		if(!this->MakeLuaRequest(data, request))
		{
			return nullptr;
		}
		std::unique_ptr<redis::Response> response = this->Run(request);
		if(response == nullptr || !response->element.IsString())
		{
			return nullptr;
		}
		std::unique_ptr<json::r::Document> document = std::make_unique<json::r::Document>();
		if(!document->Decode(response->element.message))
		{
			return nullptr;
		}
		return document;
	}

	bool RedisComponent::MakeLuaRequest(const acs::RedisLuaData& data, std::unique_ptr<redis::Request>& request)
	{
		const std::string & fullName = data.name;
		size_t pos = fullName.find('.');
		if(pos == std::string::npos)
		{
			return false;
		}
		const std::string tab = fullName.substr(0, pos);
		const std::string func = fullName.substr(pos + 1);
		auto iter = this->mLuaMap.find(tab);
		if(iter == this->mLuaMap.end())
		{
			LOG_ERROR("not find redis script {}", fullName);
			return false;
		}
		const std::string & tag = iter->second;
		request = redis::Request::MakeLua(tag, func, data.json);
		return true;
	}

	void RedisComponent::Send(std::unique_ptr<redis::Request>& request, int& rpcId)
	{
		rpcId = this->BuildRpcId();
		{
			request->SetRpcId(rpcId);
			this->Send(request);
		}
	}

	void RedisComponent::Send(int id, std::unique_ptr<redis::Request>& request) noexcept
	{
		auto iter = this->mClients.find(id);
		if(iter == this->mClients.end())
		{
			LOG_ERROR("redis:{} not exist message:{}", id, request->ToString());
			return;
		}
		//LOG_DEBUG("redis[{}] request:{}", id, request->ToString());
		iter->second->Send(request);
	}

	void RedisComponent::AddFreeClient(int id)
	{
		if(!this->mRequests.empty())
		{
			this->Send(id, this->mRequests.front());
			this->mRequests.pop();
			return;
		}
		this->mFreeClients.Push(id);
	}

	void RedisComponent::OnDestroy()
	{
		while(!this->mRequests.empty())
		{
			this->mApp->Sleep();
			LOG_DEBUG("wait redis command invoke => {}", this->mRequests.size());
		}
	}

	bool RedisComponent::UnLock(const std::string& key)
	{
		if(key.empty())
		{
			return false;
		}
		RedisLuaData redisLuaData;
		{
			json::w::Document document;
			document.Add("key", key);

			redisLuaData.name = "lock.unlock";
			document.Serialize(&redisLuaData.json);
		}
		bool result = false;
		auto response = this->CallLua(redisLuaData);
		return response && response->element.number > 0;
	}

	bool RedisComponent::Lock(const std::string& key, int timeout)
	{
		if(key.empty())
		{
			return false;
		}
		RedisLuaData redisLuaData;
		{
			json::w::Document document;
			document.Add("key", key);
			document.Add("time", timeout);
			redisLuaData.name = "lock.lock";
			document.Serialize(&redisLuaData.json);
		}
		auto response = this->CallLua(redisLuaData);
		return response && response->element.number > 0;
	}

	bool RedisComponent::Del(const std::string& key)
	{
		std::unique_ptr<redis::Response> response = this->Run("DEL", key);
		if(response == nullptr || response->HasError())
		{
			return false;
		}
		return response->element.number > 0;
	}
}
