﻿#include "ProtocolComponent.h"

#include <Core/App.h>
#include <Util/FileHelper.h>
#include <Util/StringHelper.h>
#include <google/protobuf/util/json_util.h>
#include <rapidjson/document.h>
#include <Pool/MessagePool.h>
namespace Sentry
{

	ProtocolComponent::ProtocolComponent()
	{

	}

	bool ProtocolComponent::Awake()
    {
		rapidjson::Document jsonMapper;
		const std::string & dir = App::Get().GetConfigDir();	
		if (!FileHelper::ReadJsonFile(dir + "rpc.json", jsonMapper))
		{
			SayNoDebugFatal("not find file : rpc.json");
			return false;
		}

		auto iter1 = jsonMapper.MemberBegin();
		for (; iter1 != jsonMapper.MemberEnd(); iter1++)
		{
			const std::string service = iter1->name.GetString();
			rapidjson::Value& jsonValue = iter1->value;
			SayNoAssertRetFalse_F(jsonValue.IsObject());
			SayNoAssertRetFalse_F(jsonValue.HasMember("id"));

			std::vector<ProtocolConfig *> methods;
			auto iter2 = jsonValue.MemberBegin();
			for (; iter2 != jsonValue.MemberEnd(); iter2++)
            {
                if (!iter2->value.IsObject())
                {
                    continue;
                }
                ProtocolConfig *protocolConfig = new ProtocolConfig();

				protocolConfig->Service = service;
                protocolConfig->Method = iter2->name.GetString();
                SayNoAssertRetFalse_F(iter2->value.HasMember("id"));

				protocolConfig->Async = iter2->value["async"].GetBool();
                protocolConfig->Id = (unsigned short) iter2->value["id"].GetUint();
                if (iter2->value.HasMember("request"))
                {
                    protocolConfig->Request = iter2->value["request"].GetString();
                    Message *message = MessagePool::New(protocolConfig->Request);
                    if (message == nullptr)
                    {
                        SayNoDebugFatal("create " << protocolConfig->Request << " failure");
                        return false;
                    }
                }
                if (iter2->value.HasMember("response"))
                {
                    protocolConfig->Response = iter2->value["response"].GetString();
                    Message *message = MessagePool::New(protocolConfig->Response);
                    if (message == nullptr)
                    {
                        SayNoDebugFatal("create " << protocolConfig->Response << " failure");
                        return false;
                    }
                }


                methods.push_back(protocolConfig);
                std::string name = service + "." + protocolConfig->Method;
                this->mProtocolNameMap.insert(std::make_pair(name, protocolConfig));
                this->mProtocolMap.insert(std::make_pair(protocolConfig->Id, protocolConfig));
            }
			this->mServiceMap.emplace(service, methods);
		}
        return true;
    }

	void ProtocolComponent::Start()
	{
			
	}


	void ProtocolComponent::GetServices(std::vector<std::string> & services)
	{
		auto iter = this->mServiceMap.begin();
		for (; iter != this->mServiceMap.end(); iter++)
		{
			services.push_back(iter->first);
		}
	}

	bool ProtocolComponent::GetMethods(const std::string service, std::vector<std::string> & methods)
	{
		auto iter = this->mServiceMap.find(service);
		if (iter == this->mServiceMap.end())
		{
			return false;
		}
		for (ProtocolConfig * config : iter->second)
		{
			methods.push_back(config->Method);
		}
		return true;
	}

	const ProtocolConfig *ProtocolComponent::GetProtocolConfig(unsigned short id) const
    {
        auto iter = this->mProtocolMap.find(id);
        return iter != this->mProtocolMap.end() ? iter->second : nullptr;
    }

    const ProtocolConfig *
    ProtocolComponent::GetProtocolConfig(const std::string &service, const std::string &method) const
    {
        std::string name = service + "." + method;
        auto iter = this->mProtocolNameMap.find(name);
        return iter != this->mProtocolNameMap.end() ? iter->second : nullptr;
    }
}