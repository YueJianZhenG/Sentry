//
// Created by leyi on 2024/1/18.
//

#include "XCode/XCode.h"
#include "RedisSubComponent.h"
#include "Entity/Actor/App.h"
#include "Lua/Engine/ModuleClass.h"
#include "Server/Component/ThreadComponent.h"
#include "Timer/Component/TimerComponent.h"

#include "Lua/Lib/Lib.h"

namespace acs
{
	RedisSubComponent::RedisSubComponent()
	{
		this->mIsSend = false;
		this->mTimer = nullptr;
		REGISTER_JSON_CLASS_FIELD(redis::Cluster, sub);
		REGISTER_JSON_CLASS_FIELD(redis::Cluster, retry);
	}

	bool RedisSubComponent::Awake()
	{
		LuaCCModuleRegister::Add([](Lua::CCModule & ccModule) {
			ccModule.Open("db.redis.sub", lua::lib::luaopen_lsub_redisdb);
		});
		ServerConfig::Inst()->Get("redis", this->mConfig);
		LOG_CHECK_RET_FALSE(!this->mConfig.sub.empty())
		return true;
	}

	bool RedisSubComponent::LateAwake()
	{
		ThreadComponent* component = this->GetComponent<ThreadComponent>();
		{
			redis::Config redisConfig;
			if(!redisConfig.Decode(this->mConfig.sub))
			{
				return false;
			}
			redisConfig.Get("password", redisConfig.password);
			Asio::Context & context = this->mApp->GetContext();
			LOG_CHECK_RET_FALSE(redisConfig.Get("address", redisConfig.address))
			tcp::Socket * sock = component->CreateSocket(redisConfig.address);
			this->mClient = std::make_shared<redis::Client>(1, redisConfig, this, context);
			if(!this->mClient->Start(sock))
			{
				LOG_ERROR("start sub redis fail");
				return false;
			}
		}
		this->mTimer = this->GetComponent<TimerComponent>();
		return true;
	}

	void RedisSubComponent::OnConnectOK(int id)
	{
		if(!this->mChannels.empty())
		{
			for (auto iter = this->mChannels.begin(); iter != this->mChannels.end(); iter++)
			{
				int rpcId = 0;
				const std::string & channel = *iter;
				std::unique_ptr<redis::Request> request = redis::Request::Make("SUBSCRIBE", channel);
				this->Send(request, rpcId);
			}
		}
	}

	void RedisSubComponent::OnClientError(int id, int code)
	{
		if(this->mConfig.retry > 0)
		{
			int ms = this->mConfig.retry * 1000;
			this->mTimer->Timeout(ms, [this]() {
				this->mClient->Start(nullptr);
			});
			LOG_ERROR("redis sub try connect");
		}
	}

	void RedisSubComponent::OnSecondUpdate(int tick) noexcept
	{
		if(this->mConfig.ping > 0 && tick % this->mConfig.ping == 0)
		{
			int rpcId = 0;
			std::unique_ptr<redis::Request> request
				= redis::Request::Make("PING");
			this->Send(request, rpcId);
		}
	}

	void RedisSubComponent::Send(std::unique_ptr<redis::Request>& request, int& rpcId)
	{
		rpcId = this->BuildRpcId();
		{
			request->SetRpcId(rpcId);
			if(this->mIsSend)
			{
				this->mMessages.emplace(std::move(request));
				return;
			}
			this->mIsSend = true;
			this->mClient->Send(request);
		}
	}

	bool RedisSubComponent::Sub(const std::string& channel)
	{
		int rpcId = 0;
		LOG_CHECK_RET_FALSE(!channel.empty())
		std::unique_ptr<redis::Request> request = redis::Request::Make("SUBSCRIBE", channel);
		{
			this->Send(request, rpcId);
			std::unique_ptr<redis::Response> response = this->BuildRpcTask<RedisTask>(rpcId)->Await();
			return response != nullptr && response->element.type == redis::type::Array;
		}
	}

	bool RedisSubComponent::UnSub(const std::string& channel)
	{
		int rpcId = 0;
		std::unique_ptr<redis::Request> request = redis::Request::Make("UNSUBSCRIBE", channel);
		{
			this->Send(request, rpcId);
			std::unique_ptr<redis::Response> response = this->BuildRpcTask<RedisTask>(rpcId)->Await();
			return response != nullptr && response->element.type == redis::type::Array;
		}
		return true;
	}

	void RedisSubComponent::OnMessage(int, redis::Request* req, redis::Response* resp) noexcept
	{
		do
		{
			std::unique_ptr<redis::Request> request(req);
			std::unique_ptr<redis::Response> response(resp);
			const redis::Element & element = response->element;
			if (element.type != redis::type::Array || element.list.size() != 3)
			{
				if(element.type == redis::type::Error)
				{
					LOG_ERROR("response => {}", response->ToString())
				}
				break;
			}
			auto iter = element.list.begin();
			const static std::string& option = iter->message;
			const static std::string TYPE_MESSAGE = "message";
			const static std::string TYPE_SUBSCRIBE = "subscribe";
			const static std::string TYPE_UNSUBSCRIBE = "unsubscribe";

			if(option == TYPE_MESSAGE)
			{
				const std::string& channel = (++iter)->message;
				const std::string& message = (++iter)->message;
				help::OnRedisPublishMessageEvent::Trigger(channel, message);
			}
			else if(option == TYPE_SUBSCRIBE)
			{
				const redis::Element & element1 = *(++iter);
				const redis::Element & element2 = *(++iter);
				if(element2.number > 0)
				{
					LOG_INFO("sub ({}) ok", element1.message)
					this->mChannels.emplace(element1.message);
				}
			}
			else if(option == TYPE_UNSUBSCRIBE)
			{
				const redis::Element & element1 = *(++iter);
				const redis::Element & element2 = *(++iter);
				if(element2.number == 1)
				{
					auto iter1 = this->mChannels.find(element1.message);
					if(iter1 != this->mChannels.end())
					{
						this->mChannels.erase(iter1);
						LOG_INFO("unsub ({}) ok", element1.message)
					}
				}
			}
			if(request != nullptr && request->GetRpcId() > 0)
			{
				int rpcId = request->GetRpcId();
				this->OnResponse(rpcId, response);
			}
		}
		while (false);
		this->mIsSend = false;
		if(!this->mMessages.empty())
		{
			this->mClient->Send(this->mMessages.front());
			this->mMessages.pop();
			return;
		}
		this->mClient->StartReceive();
	}
}