//
// Created by zmhy0073 on 2022/10/25.
//

#ifndef APP_REDISCONFIG_H
#define APP_REDISCONFIG_H

#include "DB/Common/Url.h"
#include <Yyjson/Object/JsonObject.h>
namespace redis
{
	struct Cluster final : public json::Object<Cluster>
    {
    public:
		int id = 1;
		int ping = 15;
		int count = 1;
		int retry = 5; //重试时间(秒)
		bool debug = false;
		int conn_count = 3;
		std::string mq;
		std::string sub;
        std::string script;
		unsigned int timeout = 0;
		std::vector<std::string> address;
	public:
		static void RegisterAllFields()
		{
			REGISTER_JSON_CLASS_FIELD(redis::Cluster, ping);
			REGISTER_JSON_CLASS_FIELD(redis::Cluster, count);
			REGISTER_JSON_CLASS_FIELD(redis::Cluster, retry);
			REGISTER_JSON_CLASS_FIELD(redis::Cluster, debug);
			REGISTER_JSON_CLASS_FIELD(redis::Cluster, script);
			REGISTER_JSON_CLASS_MUST_FIELD(redis::Cluster, address);
		}
    };

	struct Config : public db::Url
	{
	public:
		Config() : db::Url("redis") { }
	public:
		int db = 0;
		int conn_count = 3;
		std::string address;
		std::string password;
	};
}


#endif //APP_REDISCONFIG_H
