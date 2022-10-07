//
// Created by yjz on 2022/4/16.
//

#include"MongoDataComponent.h"
#include"Proto/ProtoHelper.h"
#include"Component/MysqlDataComponent.h"
#include"Component/RedisDataComponent.h"

#include"Component/MongoAgentComponent.h"
#include"Component/ProtoComponent.h"
namespace Sentry
{
	bool MongoDataComponent::LateAwake()
	{
		this->mProtoComponent = this->GetComponent<ProtoComponent>();
		this->mRedisComponent = this->GetComponent<RedisDataComponent>();
		this->mMongoComponent = this->GetComponent<MongoAgentComponent>();
		return this->mRedisComponent != nullptr && this->mMongoComponent != nullptr;
	}

	XCode MongoDataComponent::Set(long long id, std::shared_ptr<Message> message, bool insert)
	{
		std::string json;
		if (Helper::Protocol::GetJson(*message, &json))
		{
			char buffer[100] = { 0 };
			size_t size = sprintf(buffer, "{_id:%lld}", id);
			const std::string select(buffer, size);
			const std::string name = message->GetTypeName();
			XCode code = this->mMongoComponent->Update(name.c_str(), select, json, (int)id);
			if (code != XCode::Successful)
			{

			}
		}
		return XCode::Failure;
	}

	XCode MongoDataComponent::Set(const std::string& id, std::shared_ptr<Message> message, bool insert)
	{
		return XCode::Successful;
	}

	std::shared_ptr<Message> MongoDataComponent::Get(long long id, const std::string& name)
	{
		size_t pos = name.find('_');
		if (pos != std::string::npos)
		{
			const std::string db = name.substr(0, pos);
			const std::string key = name.substr(pos + 1);
			std::shared_ptr<RedisResponse> response =
				this->mRedisComponent->RunCommand(db, "HGET", key, id);
			if (response != nullptr)
			{
				switch (response->GetType())
				{
				case RedisRespType::REDIS_STRING:
				case RedisRespType::REDIS_BIN_STRING:
				{
					RedisString* redisString = (RedisString*)response->Get(0);
					std::shared_ptr<Message> result = this->mProtoComponent->New(name);
					if (util::JsonStringToMessage(redisString->GetValue(), result.get()).ok())
					{
						return result;
					}
				}
					break;
				default:
					break;
				}
			}
		}
		char buffer[100] = { 0 };
		size_t size = sprintf(buffer, "{_id:%lld}", id);
		const std::string select(buffer, size);
		std::shared_ptr<Message> result = this->mProtoComponent->New(name);
		XCode code = this->mMongoComponent->Query(name.c_str(), select, result);
		return code == XCode::Successful ? result : nullptr;
	}

	std::shared_ptr<Message> MongoDataComponent::Get(const std::string& id, const std::string& name)
	{
		size_t pos = name.find('_');
		if (pos != std::string::npos)
		{
			const std::string db = name.substr(0, pos);
			const std::string key = name.substr(pos + 1);
			std::shared_ptr<RedisResponse> response =
				this->mRedisComponent->RunCommand(db, "HGET", key, id);
			if (response != nullptr && (response->GetType() == RedisRespType::REDIS_STRING
										|| response->GetType() == RedisRespType::REDIS_BIN_STRING))
			{
				switch (response->GetType())
				{
				case RedisRespType::REDIS_STRING:
				case RedisRespType::REDIS_BIN_STRING:
				{
					RedisString* redisString = (RedisString*)response->Get(0);
					std::shared_ptr<Message> result = this->mProtoComponent->New(name);
					if (util::JsonStringToMessage(redisString->GetValue(), result.get()).ok())
					{
						return result;
					}
				}
					break;
				default:
					break;
				}
			}
		}
		char buffer[100] = { 0 };
		size_t size = sprintf(buffer, "{_id:%s}", id.c_str());
		const std::string select(buffer, size);
		std::shared_ptr<Message> result = this->mProtoComponent->New(name);
		XCode code = this->mMongoComponent->Query(name.c_str(), select, result);
		return code == XCode::Successful ? result : nullptr;
	}
}