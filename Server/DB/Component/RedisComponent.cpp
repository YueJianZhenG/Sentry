﻿#include"RedisComponent.h"

#include"Object/App.h"
#include"Util/FileHelper.h"
#include"Util/DirectoryHelper.h"
#include"Script/ClassProxyHelper.h"
#include"Scene/RpcConfigComponent.h"
#include"Scene/ThreadPoolComponent.h"
#include"Service/SubService.h"
#include"RedisClient/NetWork/RedisClient.h"
namespace Sentry
{
    bool RedisComponent::Awake()
    {
        std::string path;
        this->mRedisConfig.mCount = 3;
        const ServerConfig &config = App::Get().GetConfig();
        config.GetValue("redis", "count", this->mRedisConfig.mCount);
        config.GetValue("redis", "lua", this->mRedisConfig.mLuaFilePath);
        config.GetValue("redis", "passwd", this->mRedisConfig.mPassword);
        LOG_CHECK_RET_FALSE(config.GetValue("redis", "ip", this->mRedisConfig.mIp));
        LOG_CHECK_RET_FALSE(config.GetValue("redis", "port", this->mRedisConfig.mPort));
        return true;
    }

    std::shared_ptr<RedisResponse>
    RedisComponent::Call(const std::string &tab, const std::string &func, std::vector<std::string> &args)
    {
        std::string script;
        if(!this->GetLuaScript(fmt::format("{0}.lua", tab), script))
        {
            LOG_ERROR("not find redis script ", fmt::format("{0}.lua", tab));
            return nullptr;
        }
        std::shared_ptr<RedisRequest> redisCmdRequest(new RedisRequest("EVALSHA"));
        redisCmdRequest->InitParameter(script, (int) args.size() + 1, func);
        for(const std::string & val : args)
        {
            redisCmdRequest->AddParameter(val);
        }
        return this->InvokeCommand(redisCmdRequest);
    }

    std::shared_ptr<RedisResponse> RedisComponent::InvokeCommand(std::shared_ptr<RedisRequest> request)
    {
        std::shared_ptr<RedisClient> redisClient = this->AllotRedisClient();
#ifdef __DEBUG__
        ElapsedTimer elapsedTimer;
        LOG_WARN("redis command name = ", redisClient->GetName());
#endif
        if (redisClient == nullptr)
        {
            LOG_ERROR("allot redis client failure");
            return nullptr;
        }
        auto response = redisClient->InvokeCommand(request)->Await();
#ifdef __DEBUG__
        LOG_INFO("invoke redis command use time : ", elapsedTimer.GetMs(), "ms");
#endif
        if (!this->mWaitAllotClients.empty())
        {
            auto taskClient = this->mWaitAllotClients.front();
            this->mWaitAllotClients.pop();
            taskClient->SetResult(redisClient);
            return response;
        }
        this->mFreeClients.emplace(redisClient);
        return response;
    }

    bool RedisComponent::LoadLuaScript(const std::string &path)
    {
        std::string content;
        if(!Helper::File::ReadTxtFile(path, content))
        {
            return false;
        }
        auto response = this->InvokeCommand("script", "load", content);
        if(response->HasError())
        {
            LOG_ERROR(response->GetValue());
            return false;
        }
        std::string fileName;
        Helper::Directory::GetDirAndFileName(path, content, fileName);
        LOG_WARN(fileName, "  ", response->GetValue());
        this->mLuaCommandMap.emplace(fileName,  response->GetValue());
        return true;
    }

    bool RedisComponent::GetLuaScript(const std::string &file, std::string &command)
    {
        auto iter = this->mLuaCommandMap.find(file);
        if(iter != this->mLuaCommandMap.end())
        {
            command = iter->second;
            return true;
        }
        return false;
    }

    void RedisComponent::OnStart()
    {
        std::string path;
        std::vector<std::string> luaFiles;
        App::Get().GetConfig().GetValue("redis", "lua", path);
        Helper::Directory::GetFilePaths(path, "*.lua", luaFiles);

        this->mSubRedisClient = this->MakeRedisClient("Subscribe");
        if (this->mSubRedisClient != nullptr)
        {
            this->SubscribeMessage();
            LOG_DEBUG("subscribe redis client connect successful");
            this->mTaskComponent->Start(&RedisComponent::StartPubSub, this);
        }

        for (size_t index = 0; index < this->mRedisConfig.mCount; index++)
        {
            string name = fmt::format("Command:{0}", index + 1);
            std::shared_ptr<RedisClient> redisClient = this->MakeRedisClient(name);
            if(redisClient == nullptr)
            {
                LOG_ERROR("connect redis ", this->mRedisConfig.mIp, ':', this->mRedisConfig.mPort, " failure");
                return;
            }
            this->mFreeClients.emplace(redisClient);
        }

        for (const std::string &file: luaFiles)
        {
            LOG_CHECK_RET(this->LoadLuaScript(file));
            LOG_INFO("load redis script ", file, " successful");
        }
        this->mTaskComponent->Start(&RedisComponent::CheckRedisClient, this);
    }

    std::shared_ptr<RedisClient> RedisComponent::MakeRedisClient(const std::string & name)
    {
        NetWorkThread &workThread = this->mThreadComponent->AllocateNetThread();
        auto socketProxy = std::make_shared<SocketProxy>(workThread, name);
        auto redisCommandClient = std::make_shared<RedisClient>(socketProxy);

        for (size_t index = 0; index < 3; index++)
        {
            if (redisCommandClient->ConnectAsync(this->mRedisConfig.mIp, this->mRedisConfig.mPort)->Await())
            {
                if(!this->mRedisConfig.mPassword.empty())
                {
                    std::shared_ptr<RedisRequest> request(new RedisRequest("AUTH"));
                    request->AddParameter(this->mRedisConfig.mPassword);
                    auto response = redisCommandClient->InvokeCommand(request)->Await();
                    if(!response->IsOk())
                    {
                        LOG_ERROR("auth redis passwork error : ", this->mRedisConfig.mPassword);
                        return nullptr;
                    }
                }
                return redisCommandClient;
            }
        }
        return nullptr;
    }

    std::shared_ptr<RedisClient> RedisComponent::AllotRedisClient()
    {
        std::shared_ptr<RedisClient> redisClient;
        if (this->mFreeClients.empty())
        {
            std::shared_ptr<TaskSource<std::shared_ptr<RedisClient>>>
                    taskSource(new TaskSource<std::shared_ptr<RedisClient>>());
            this->mWaitAllotClients.push(taskSource);
            redisClient = taskSource->Await();
        }
        else
        {
            redisClient = this->mFreeClients.front();
            this->mFreeClients.pop();
        }


        if (!redisClient->IsOpen())
        {
            const std::string &ip = this->mRedisConfig.mIp;
            unsigned short port = this->mRedisConfig.mPort;
            if (!redisClient->ConnectAsync(ip, port)->Await())
            {
                return nullptr;
            }
            if (!this->mRedisConfig.mPassword.empty())
            {
                std::shared_ptr<RedisRequest> request(new RedisRequest("AUTH"));
                request->AddParameter(this->mRedisConfig.mPassword);
                auto response = redisClient->InvokeCommand(request)->Await();
                if (!response->IsOk())
                {
                    LOG_ERROR("auth redis passwork error : ", this->mRedisConfig.mPassword);
                    return nullptr;
                }
            }
        }
        return redisClient;
    }

    long long RedisComponent::Publish(const std::string &channel, const std::string &message)
    {
        return this->InvokeCommand("PUBLISH", channel, message)->GetNumber();
    }

    void RedisComponent::CheckRedisClient()
    {
        ElapsedTimer timer;
        std::vector<TaskContext *> tasks;
        for (int index = 0; index < 100; index++)
        {
            string key = fmt::format("key:{0}", index);
            tasks.push_back(this->mTaskComponent->Start(&RedisComponent::AddCorouterNum, this, key));
        }
        this->mTaskComponent->WhenAll(tasks);
        LOG_ERROR("user time = ", timer.GetMs(), "ms");
    }

    void RedisComponent::AddCorouterNum(const std::string &key)
    {
        long long value = this->AddCounter(key);
    }

    bool RedisComponent::SubscribeChannel(const std::string &chanel)
    {
        std::shared_ptr<RedisRequest> request(new RedisRequest("SUBSCRIBE"));
        request->AddParameter(std::move(chanel));
        auto response = this->mSubRedisClient->InvokeCommand(request)->Await();
        return !response->HasError();
    }

    void RedisComponent::SubscribeMessage()
    {
        std::vector<Component *> components;
        this->GetComponents(components);
        for (Component *component: components)
        {
            auto subService = dynamic_cast<SubService *>(component);
            if (subService == nullptr)
            {
                continue;
            }
            std::vector<std::string> methods;
            subService->GetSubMethods(methods);
            for (const std::string &name: methods)
            {
                const std::string &service = subService->GetName();
                if (this->SubscribeChannel(fmt::format("{0}.{1}", service, name)))
                {
                    LOG_INFO("subscribe chanel [", fmt::format("{0}.{1}", service, name), "] successful");
                }
            }
        }
    }

    void RedisComponent::StartPubSub()
    {
        while (this->mSubRedisClient)
        {
            if (!this->mSubRedisClient->IsOpen())
            {
                int count = 0;
                const std::string &ip = this->mRedisConfig.mIp;
                unsigned short port = this->mRedisConfig.mPort;
                while (!this->mSubRedisClient->ConnectAsync(ip, port)->Await())
                {
                    LOG_ERROR("connect redis [", ip, ':', port, "] failure count = ", count++);
                    this->mTaskComponent->Sleep(3000);
                }
                if (!this->mRedisConfig.mPassword.empty())
                {
                    std::shared_ptr<RedisRequest> request(new RedisRequest("AUTH"));
                    request->AddParameter(this->mRedisConfig.mPassword);
                    auto response = this->mSubRedisClient->InvokeCommand(request)->Await();
                    if (response == nullptr || response->IsOk())
                    {
                        LOG_FATAL("auth redis passwd error", this->mRedisConfig.mPassword);
                        return;
                    }
                }
            }

            auto redisResponse = this->mSubRedisClient->WaitRedisMessageResponse()->Await();
            if (!redisResponse->HasError() && redisResponse->GetArraySize() == 3 &&
                redisResponse->GetValue() == "message")
            {
                const std::string &method = redisResponse->GetValue(1);
                size_t pos = method.find('.');
                if(pos != std::string::npos)
                {
                    string service = method.substr(0, pos);
                    std::string funcName = method.substr(pos + 1);
                    const std::string &message = redisResponse->GetValue(2);
#ifdef __DEBUG__
                    LOG_DEBUG("========= subscribe message =============");
                    LOG_DEBUG("func = ", service, '.', method);
                    LOG_DEBUG("message = ", message);
#endif
                    auto subService = this->GetComponent<SubService>(service);
                    if (subService != nullptr)
                    {
                        subService->Publish(funcName, message);
                    }
                }
            }
        }
    }


    bool RedisComponent::LateAwake()
    {
        this->mTaskComponent = this->GetComponent<TaskComponent>();
        this->mThreadComponent = this->GetComponent<ThreadPoolComponent>();
        return true;
    }


    long long RedisComponent::AddCounter(const string &key)
    {
       return this->InvokeCommand("INCR", key)->GetNumber();
    }
}
