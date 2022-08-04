//
// Created by zmhy0073 on 2022/8/3.
//

#include"RedisSubComponent.h"
#include"Script/Function.h"
#include"Script/Extension/Json/Json.h"
#include"Component/Lua/LuaScriptComponent.h"
#include"Component/Scene/NetEventComponent.h"
namespace Sentry
{
    bool RedisSubComponent::LateAwake()
    {
        LOG_CHECK_RET_FALSE(RedisComponent::LateAwake());
        const ServerConfig & config = this->GetApp()->GetConfig();
        this->mLuaComponent = this->GetComponent<LuaScriptComponent>();
        const rapidjson::Value * jsonValue = config.GetJsonValue("redis", "main");
        if(jsonValue == nullptr)
        {
            LOG_ERROR("not find main redis config");
            return false;
        }
		config.GetListener("rpc", this->mAddress);
		return this->ParseConfig("main", *jsonValue) != nullptr;
    }

    bool RedisSubComponent::OnStart()
    {
        this->mSubClient = this->MakeRedisClient("main");
        std::shared_ptr<RedisRequest> request = RedisRequest::Make("PING");
        if(this->Run(this->mSubClient, request) != nullptr)
        {
            this->mSubClient->StartReceiveMessage();
            return true;
        }
        return false;
    }

    void RedisSubComponent::OnNotFindResponse(long long taskId, std::shared_ptr<RedisResponse> response)
    {
        if(response->GetType() == RedisRespType::REDIS_ARRAY && response->GetArraySize() == 3)
        {
            const RedisAny * redisAny1 = response->Get(0);
            const RedisAny * redisAny2 = response->Get(1);
            const RedisAny * redisAny3 = response->Get(2);
            if(redisAny1->IsString() && redisAny2->IsString() && redisAny3->IsString())
            {
                if(static_cast<const RedisString*>(redisAny1)->GetValue() == "message")
                {
                    std::vector<std::string> methodInfos;
                    const std::string & channel = redisAny2->Cast<RedisString>()->GetValue();
                    const std::string & message = redisAny3->Cast<RedisString>()->GetValue();
                    google::protobuf::SplitStringUsing(channel, ".", &methodInfos);
					if (methodInfos.size() != 2)
					{
                        LOG_INFO("handler redis event json = " << redisAny3->Cast<RedisString>()->GetValue());
                        return;
					}
                    const std::string & method = methodInfos[1];
                    const std::string & component = methodInfos[0];
                    if(this->mLuaComponent != nullptr)
                    {
                        lua_State * lua = this->mLuaComponent->GetLuaEnv();
                        if(Lua::Function::Get(lua, component.c_str(), method.c_str()))
                        {
                            Lua::Json::Write(lua, message);
                            if(lua_pcall(lua, 1, 0, 0) != LUA_OK)
                            {
                                LOG_ERROR("handler lua event " << channel << " error = " << lua_tostring(lua, -1));
                            }
                            return;
                        }
                    }

                    NetEventComponent* localServiceComponent = this->GetComponent<NetEventComponent>(component);
                    if(localServiceComponent != nullptr)
                    {
                        std::shared_ptr<Json::Reader> jsonReader(new Json::Reader());
                        if(!localServiceComponent->Invoke(method, jsonReader))
                        {
                            LOG_ERROR("handler event " << channel << " error");
                            return;
                        }
                        LOG_INFO("handler redis event json = " << redisAny3->Cast<RedisString>()->GetValue());
                    }
                }
            }
        }
		this->mSubClient->StartReceiveMessage();
    }

    long long RedisSubComponent::Publish(const std::string& channel, const std::string& message)
    {
        std::shared_ptr<RedisRequest> request = RedisRequest::Make("PUBLISH", channel, message);
        std::shared_ptr<RedisResponse> redisResponse = this->Run(this->mSubClient, request);
        return redisResponse == nullptr ? 0 : redisResponse->GetNumber();
    }

    bool RedisSubComponent::SubscribeChannel(const std::string& channel)
    {
        std::shared_ptr<RedisRequest> request = RedisRequest::Make("SUBSCRIBE", channel);
        std::shared_ptr<RedisResponse> redisResponse = this->Run(this->mSubClient, request);
        if(redisResponse->GetArraySize() == 3 && redisResponse->Get(2)->IsNumber())
        {
            if(((const RedisNumber*)redisResponse->Get(2))->GetValue() > 0)
            {
                LOG_INFO("sub " << channel << " successful");
                return true;
            }
        }
        LOG_INFO("sub " << channel << " failure");
        return false;
    }
}