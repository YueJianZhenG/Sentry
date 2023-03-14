﻿#pragma once

#include"Guid/Guid.h"
#include"Json/JsonWriter.h"
#include"Timer/ElapsedTimer.h"
#include"Client/TcpRedisClient.h"
#include"Component/RpcTaskComponent.h"

namespace Sentry
{
	class ThreadComponent;

    class RedisComponent final : public RpcTaskComponent<int, RedisResponse>,
								 public ILuaRegister, public IStart, public IRpc<RedisResponse>
	{
	 public:
		RedisComponent() = default;
    public:
		bool Ping(size_t index);
		bool Send(std::shared_ptr<RedisRequest> request);
		bool Send(std::shared_ptr<RedisRequest> request, int & id);
    public:
        template<typename ... Args>
        bool Send(const std::string & cmd, Args&& ... args);
        template<typename ... Args>
        std::shared_ptr<RedisResponse> Run(const std::string & cmd, Args&& ... args);
    public:
        std::shared_ptr<RedisResponse> Run(std::shared_ptr<RedisRequest> request);
    private:
		TcpRedisClient * GetClient(size_t index = -1);
		void OnConnectSuccessful(const std::string &address) final;
        void OnMessage(std::shared_ptr<RedisResponse> message) final;
		TcpRedisClient * MakeRedisClient(const RedisClientConfig & config);
    private:
        bool Awake() final;
        bool Start() final;
        bool LateAwake() final;
		void OnLuaRegister(Lua::ClassProxyHelper &luaRegister) final;
		void OnTaskComplate(int key) final { this->mNumberPool.Push(key);}
	private:
        Util::NumberBuilder<int, 1> mNumberPool;
        std::vector<std::shared_ptr<TcpRedisClient>> mRedisClients;
    };

    template<typename ... Args>
    std::shared_ptr<RedisResponse> RedisComponent::Run(const std::string & cmd, Args &&...args)
    {
        std::shared_ptr<RedisRequest> request = std::make_shared<RedisRequest>(cmd);
        RedisRequest::InitParameter(request, std::forward<Args>(args)...);
        return this->Run(request);
    }

    template<typename ... Args>
    bool RedisComponent::Send(const std::string &cmd, Args &&...args)
    {
        TcpRedisClient * redisClient = this->GetClient();
        if(redisClient == nullptr)
        {
            return false;
        }
        std::shared_ptr<RedisRequest> request = std::make_shared<RedisRequest>(cmd);
        RedisRequest::InitParameter(request, std::forward<Args>(args)...);
		redisClient->Send(request);
        return true;
    }
}