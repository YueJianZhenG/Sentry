﻿#pragma once

#include"Util/Guid.h"
#include"Other/ElapsedTimer.h"
#include"Component/Component.h"
#include"Coroutine/TaskComponent.h"
#include"DB/RedisClient/NetWork/RedisClient.h"
using namespace Sentry;
namespace Sentry
{
	class ThreadPoolComponent;

    struct RedisConfig
    {
    public:
        int mCount;
        std::string mIp;
        unsigned short mPort;
        std::string mPassword;
        std::string mLuaFilePath;
    };

    class RedisComponent : public Component, public IStart
	{
	public:
		RedisComponent() = default;
		~RedisComponent() final = default;
    public:
        void ClearAllData();
		bool CloseRedisSocket();

	protected:
		bool Awake() final;            //初始化管理器
		bool LateAwake() final;                //初始化完成之后
        void OnStart() override;
    private:
        void StartPubSub();
        void SubscribeMessage();
        void CheckRedisClient();
	public:
        long long AddCounter(const std::string & key);

        bool SubscribeChannel(const std::string & chanel);

        long long Publish(const std::string & channel, const std::string & message);

        template<typename ... Args>
        std::shared_ptr<RedisResponse> InvokeCommand(const std::string & cmd, Args && ... args)
        {
            std::shared_ptr<RedisRequest> request(new RedisRequest(cmd));
            request->InitParameter(std::forward<Args>(args) ...);
            return this->InvokeCommand(request);
        }

        std::shared_ptr<RedisResponse> InvokeCommand(std::shared_ptr<RedisRequest> request);


        template<typename ... Args>
        std::shared_ptr<RedisResponse> Call(const std::string & tab, const std::string & func, Args &&...args)
        {
            std::string script;
            int size = sizeof ...(Args) + 1;
            if(!this->GetLuaScript(fmt::format("{0}.lua", tab), script))
            {
                LOG_ERROR("not find redis script ", fmt::format("{0}.lua", tab));
                return nullptr;
            }
            return this->InvokeCommand("EVALSHA",script, size, func, std::forward<Args>(args)...);
        }

        std::shared_ptr<RedisResponse> Call(const std::string & tab, const std::string & func, std::vector<std::string> & args);

    private:
        bool LoadLuaScript(const std::string & path);
        std::shared_ptr<RedisClient> AllotRedisClient();
        bool GetLuaScript(const std::string & file, std::string & command);
        std::shared_ptr<RedisClient> MakeRedisClient(const std::string & name);
    private:
        RedisConfig mRedisConfig;
        TaskComponent * mTaskComponent;
        ThreadPoolComponent * mThreadComponent;
        std::shared_ptr<RedisClient> mSubRedisClient;
        std::queue<std::shared_ptr<RedisClient>> mFreeClients;
        std::unordered_map<std::string, std::string> mLuaCommandMap;
        std::queue<TaskSourceShared<RedisClient>> mWaitAllotClients;
        //std::unordered_map<std::thread::id, redisContext *> mRedisContextMap;
	};
}