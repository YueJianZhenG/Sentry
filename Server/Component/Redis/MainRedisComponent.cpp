﻿#include"MainRedisComponent.h"

#include"App/App.h"
#include"Util/FileHelper.h"
#include"Util/DirectoryHelper.h"
#include"Script/ClassProxyHelper.h"
#include"Component/Service/SubService.h"
#include"Global/RpcConfig.h"
#include"Component/Scene/ThreadPoolComponent.h"
#include"DB/Redis/RedisClient.h"
namespace Sentry
{
	bool MainRedisComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->LoadRedisConfig());
		this->mTaskComponent = this->GetComponent<TaskComponent>();
		this->mTimerComponent = this->GetComponent<TimerComponent>();
		LOG_CHECK_RET_FALSE(this->GetComponent<ThreadPoolComponent>());
		this->GetConfig().GetListenerAddress("rpc", this->mRpcAddress);
		return true;
	}
	bool MainRedisComponent::LoadRedisConfig()
	{
		this->mConfig = this->GetConfig().GetRedisConfig("main");
		return this->mConfig != nullptr;
	}

	bool MainRedisComponent::GetLuaScript(const std::string& file, std::string& command)
	{
		auto iter = this->mLuaCommandMap.find(file);
		if (iter != this->mLuaCommandMap.end())
		{
			command = iter->second;
			return true;
		}
		return false;
	}

	bool MainRedisComponent::OnStart()
	{
		this->mRedisClient = this->MakeRedisClient();
		this->mSubRedisClient = this->MakeRedisClient();
		if (this->mSubRedisClient == nullptr || this->mRedisClient == nullptr)
		{
			LOG_ERROR("connect redis " << this->mConfig->Address << " failure");
			return false;
		}

#ifdef __DEBUG__
		//this->Run("FLUSHALL")->IsOk();
		std::shared_ptr<RedisRequest> request = RedisRequest::Make("FLUSHALL");
		std::shared_ptr<RedisResponse> response = std::make_shared<RedisResponse>();
		if(this->mRedisClient->Run(request, response) != XCode::Successful || !response->IsOk())
		{
			return false;
		}
#endif

		for (const std::string& path: this->mConfig->LuaFiles)
		{
			std::string key;
			if (!this->mRedisClient->LoadLuaScript(path, key))
			{
				return false;
			}
			std::string fileName, director;
			if (!Helper::Directory::GetDirAndFileName(path, director, fileName))
			{
				return false;
			}
			LOG_WARN(fileName << "  " << key);
			this->mLuaCommandMap.emplace(fileName, key);
			LOG_INFO("load redis script " << path << " successful");
		}

		auto iter = this->mLuaCommandMap.begin();
		for(; iter != this->mLuaCommandMap.end(); iter++)
		{
			const std::string & file = iter->first;
			const std::string & value = iter->second;
			std::shared_ptr<RedisResponse> response(new RedisResponse());
			std::shared_ptr<RedisRequest> request = RedisRequest::Make("HSET", "lua", file, value);
			if(this->mRedisClient->Run(request, response) != XCode::Successful)
			{
				return false;
			}
		}

		this->SubscribeMessage();
		this->mTaskComponent->Start(&MainRedisComponent::StartPubSub, this);
		this->mTaskComponent->Start(&MainRedisComponent::CheckRedisClient, this);
		return true;
	}

	std::shared_ptr<RedisClient> MainRedisComponent::MakeRedisClient()
	{
#ifdef ONLY_MAIN_THREAD
		IAsioThread& workThread = App::Get()->GetTaskScheduler();
#else
		ThreadPoolComponent * threadPoolComponent = this->GetComponent<ThreadPoolComponent>();
		IAsioThread& workThread = threadPoolComponent->AllocateNetThread();
#endif
		size_t count = 0;
		const std::string & ip = this->mConfig->Ip;
		unsigned short port = this->mConfig->Port;
		std::shared_ptr<SocketProxy> socketProxy = std::make_shared<SocketProxy>(workThread, ip, port);
		std::shared_ptr<RedisClient> redisCommandClient = std::make_shared<RedisClient>(socketProxy, this->mConfig);

		XCode code = redisCommandClient->StartConnect();
		if(code == XCode::RedisAuthFailure)
		{
			LOG_ERROR("redis auth failure");
			return nullptr;
		}

		while(code != XCode::Successful)
		{
			LOG_ERROR(this->mConfig->Name << " connect redis ["
										  << this->mConfig->Address << "] failure count = " << count++);
			this->mTaskComponent->Sleep(3000);
			code = redisCommandClient->StartConnect();
		}
		LOG_INFO(this->mConfig->Name << " connect redis [" << this->mConfig->Address << "] successful");
		return redisCommandClient;
	}

	long long MainRedisComponent::Publish(const std::string& channel, const std::string& message)
	{
		std::shared_ptr<RedisResponse> response = std::make_shared<RedisResponse>();
		std::shared_ptr<RedisRequest> request = RedisRequest::Make("PUBLISH", channel, message);
		if(this->mRedisClient->Run(request, response) != XCode::Successful)
		{
			LOG_ERROR("redis net work error publish error");
			return 0;
		}
		return response->GetNumber();
	}

	long long MainRedisComponent::Publish(const std::string address, const string& func, Json::Writer& jsonWriter)
	{
		jsonWriter.AddMember("func", func);
		return this->Publish(address, jsonWriter);
	}

	long long MainRedisComponent::Publish(const std::string& channel, Json::Writer& jsonWriter)
	{
		std::string json = jsonWriter.ToJsonString();
#ifdef __DEBUG__
		LOG_INFO("==== redis publish message ====");
		LOG_INFO("channel = " << channel);
		LOG_INFO("message = " << json);
#endif
		return this->Publish(channel, json);
	}

	void MainRedisComponent::CheckRedisClient()
	{
		while(this->mSubRedisClient != nullptr)
		{
			this->mTaskComponent->Sleep(5000);
			std::shared_ptr<RedisRequest> request = RedisRequest::Make("HGET", "lua", "json.lua");
			std::shared_ptr<RedisResponse> response = std::make_shared<RedisResponse>();
			if(this->mRedisClient->Run(request, response) != XCode::Successful)
			{

			}
		}
	}

	bool MainRedisComponent::SubscribeChannel(const std::string& channel)
	{
		assert(!channel.empty());
		std::shared_ptr<RedisResponse> response = std::make_shared<RedisResponse>();
		std::shared_ptr<RedisRequest> request = RedisRequest::Make("SUBSCRIBE", channel);
		if(this->mSubRedisClient->Run(request, response) != XCode::Successful)
		{
			LOG_ERROR("redis net work error sub " << channel << " failure");
			return false;
		}
		if(response->GetNumber() > 0)
		{
			LOG_INFO("subscribe channel [" << channel << "] successful");
			return true;
		}
		LOG_ERROR("subscribe channel [" << channel << "] failure : " << response->GetNumber());
		return false;
	}

	void MainRedisComponent::SubscribeMessage()
	{
		std::vector<Component *> components;
		this->GetApp()->GetComponents(components);
		for(Component * component : components)
		{
			SubService * subService = component->Cast<SubService>();
			if(subService != nullptr && subService->IsStartService())
			{
				std::vector<std::string> methods;
				subService->GetSubMethods(methods);
				for (const std::string& name : methods)
				{
					const std::string& service = subService->GetName();
					this->SubscribeChannel(fmt::format("{0}.{1}", service, name));
				}
			}
		}
		this->SubscribeChannel(this->mRpcAddress);
	}

	void MainRedisComponent::StartPubSub()
	{
		while (this->mSubRedisClient)
		{
			if (!this->mSubRedisClient->IsOpen())
			{
				int count = 0;
				XCode code = this->mSubRedisClient->StartConnect();
				if (code == XCode::RedisAuthFailure)
				{
					LOG_FATAL("sub client auth failure");
					return;
				}
				while (code != XCode::Successful)
				{
					LOG_ERROR("subclient connect redis "
							<< this->mConfig->Address << " failure count = " << count++);
					this->mTaskComponent->Sleep(3000);
					code = this->mSubRedisClient->StartConnect();
				}
				this->SubscribeMessage();
				LOG_DEBUG("subscribe redis client connect successful");
			}
			std::shared_ptr<RedisResponse> redisResponse(new RedisResponse());
			XCode code = this->mSubRedisClient->WaitRedisResponse(redisResponse);
			if (code != XCode::Successful)
			{
				LOG_FATAL("sub redis client error");
				continue;
			}

			std::string type;
			if (!redisResponse->HasError() && redisResponse->GetArraySize() == 3 &&
				redisResponse->GetString(type) && type == "message")
			{
				std::string channel, message;
				redisResponse->GetString(channel, 1);
				redisResponse->GetString(message, 2);
#ifdef __DEBUG__
				LOG_DEBUG("========= subscribe message =============");
				LOG_DEBUG("channel = " << channel);
				LOG_DEBUG("message = " << message);
#endif
				if (!this->HandlerSubMessage(channel, message))
				{
					LOG_FATAL(channel << " handler failure");
				}
			}
		}

	}

	bool MainRedisComponent::HandlerSubMessage(const std::string& channel, const std::string & message)
	{
		std::shared_ptr<Json::Reader> jsonReader(new Json::Reader());
		if (!jsonReader->ParseJson(message))
		{
			LOG_ERROR("parse sub message error");
			return false;
		}

		std::string service;
		std::string funcName;
		if (channel == this->mRpcAddress)
		{
			std::string fullName;
			jsonReader->GetMember("func", fullName);
			size_t pos = fullName.find('.');
			if (pos == std::string::npos)
			{
				LOG_ERROR(fullName << " parse error");
				return false;
			}
			service = fullName.substr(0, pos);
			funcName = fullName.substr(pos + 1);
		}
		else
		{
			size_t pos = channel.find('.');
			if (pos == std::string::npos)
			{
				LOG_ERROR(channel << " parse error");
				return false;
			}
			service = channel.substr(0, pos);
			funcName = channel.substr(pos + 1);
		}

		SubService* subService = this->GetComponent<SubService>(service);
		if (subService == nullptr)
		{
			LOG_ERROR("not find " << service);
			return false;
		}
		return subService->Invoke(funcName, *jsonReader);
	}

	long long MainRedisComponent::AddCounter(const string& key)
	{
		std::shared_ptr<RedisResponse> response(new RedisResponse());
		std::shared_ptr<RedisRequest> request = RedisRequest::Make("INCR", key);
		if(this->mRedisClient->Run(request, response) != XCode::Successful)
		{
			return 0;
		}
		return response->GetNumber();
	}

	bool MainRedisComponent::Lock(const string& key)
	{
		std::string tag;
		if (!this->GetLuaScript("lock.lua", tag))
		{
			LOG_ERROR("not find redis script lock.lua");
			return false;
		}
		Json::Writer jsonWriter;
		std::list<std::string> keys{ "key", "time" };
		std::list<std::string> values{ key, std::to_string(5) };
		std::shared_ptr<RedisResponse> response(new RedisResponse());
		std::shared_ptr<RedisRequest> request = RedisRequest::MakeLua(tag, "lock", keys, values);
		if (this->mRedisClient->Run(request, response) != XCode::Successful || !response->IsOk())
		{
			LOG_ERROR("redis socket error get redis lock failure");
			return false;
		}
		//LOG_DEBUG("redis lock " << key << " get successful");
		this->mLockTimers[key] = this->mTimerComponent->DelayCall(4.5f, &MainRedisComponent::OnLockTimeout, this, key);
		return true;
	}

	bool MainRedisComponent::UnLock(const string& key)
	{
		auto iter = this->mLockTimers.find(key);
		if(iter != this->mLockTimers.end())
		{
			unsigned int id = iter->second;
			this->mLockTimers.erase(iter);
			this->mTimerComponent->CancelTimer(id);
		}
		std::shared_ptr<RedisRequest> request = RedisRequest::Make("DEL", key);
		std::shared_ptr<RedisResponse> response = std::make_shared<RedisResponse>();
		if(this->mRedisClient->Run(request, response) != XCode::Successful)
		{
			return false;
		}
		LOG_DEBUG("redis lock " << key << " unlock");
		return true;
	}

	void MainRedisComponent::OnLockTimeout(const std::string& key)
	{
		auto iter = this->mLockTimers.find(key);
		if (iter != this->mLockTimers.end())
		{
			this->mLockTimers.erase(iter);
			this->mTaskComponent->Start([this, key]()
			{
				std::shared_ptr<RedisResponse> response(new RedisResponse());
				std::shared_ptr<RedisRequest> request = RedisRequest::Make("SETEX", key, 5, 1);
				if(this->mRedisClient->Run(request, response) == XCode::Successful && response->IsOk())
				{
					this->mLockTimers[key] = this->mTimerComponent->DelayCall(4.5f, &MainRedisComponent::OnLockTimeout,
							this, key);
					return;
				}
				LOG_ERROR("redis lock " << key << " delay failure");
			});
		}
	}

}