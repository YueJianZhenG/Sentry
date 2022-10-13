//
// Created by mac on 2022/5/18.
//

#ifndef SERVER_REDISBASECOMPONENT_H
#define SERVER_REDISBASECOMPONENT_H
#include"Client/TcpRedisClient.h"
#include"Component/RpcTaskComponent.h"

namespace Sentry
{
    class RedisLuaResponse
    {
    public:
        bool GetResult();
        bool ParseJson(const std::string & json);
        const Json::Reader & GetData() const { return this->mJson; }
    private:
        Json::Reader mJson;
    };
}

namespace Sentry
{
	struct RedisConfig;

    class RedisComponent : public RpcTaskComponent<RedisResponse>
	{
	public:
		RedisComponent() = default;
	 protected:
        bool LoadConfig();
        TcpRedisClient * MakeRedisClient(const RedisConfig & config);
        bool ParseConfig(const char * name, const rapidjson::Value & json);
    public:
        bool StartConnectRedis();
        bool Ping(TcpRedisClient * redisClient);
		virtual TcpRedisClient * GetClient(const std::string & name);
        virtual void OnLoadScript(const std::string & name, const std::string & md5) { }
        std::shared_ptr<RedisResponse> Run(const std::string & name, std::shared_ptr<RedisRequest> request);
    protected:
        virtual bool OnInitRedisClient(RedisConfig config) = 0;
        std::shared_ptr<RedisResponse> Run(TcpRedisClient * redisClientContext, std::shared_ptr<RedisRequest> request);
    private:
        class NetThreadComponent * mNetComponent;
        std::unordered_map<std::string, RedisConfig> mConfigs;
        std::unordered_map<std::string, std::vector<std::shared_ptr<TcpRedisClient>>> mRedisClients;
	};
}


#endif //SERVER_REDISBASECOMPONENT_H
