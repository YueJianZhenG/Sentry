﻿#include "RpcProtoComponent.h"

#include <Core/App.h>
#include <Util/FileHelper.h>
#include <Util/StringHelper.h>
#include <rapidjson/document.h>
#include <Pool/MessagePool.h>
#include <google/protobuf/util/json_util.h>
namespace GameKeeper
{
	bool RpcProtoComponent::Awake()
    {
        return this->OnLoadConfig();
    }

    bool RpcProtoComponent::OnLoadConfig()
    {
        rapidjson::Document jsonMapper;
        const std::string path = App::Get().GetConfigPath() + "rpc.json";
        if (!FileHelper::ReadJsonFile(path, jsonMapper))
        {
            GKDebugFatal("not find file : " << path << "");
            return false;///
        }

        this->mServiceMap.clear();
        this->mProtocolNameMap.clear();

        auto iter1 = jsonMapper.MemberBegin();
        for (; iter1 != jsonMapper.MemberEnd(); iter1++)
        {
            const std::string service = iter1->name.GetString();
            rapidjson::Value &jsonValue = iter1->value;
            GKAssertRetFalse_F(jsonValue.IsObject());

            std::vector<ProtocolConfig> methods;
            auto iter2 = jsonValue.MemberBegin();
            for (; iter2 != jsonValue.MemberEnd(); iter2++)
            {
                if (!iter2->value.IsObject())
                {
                    continue;
                }
                ProtocolConfig protocolConfig;
                protocolConfig.Service = service;
                protocolConfig.Method = iter2->name.GetString();
                protocolConfig.MethodId = iter2->value["Id"].GetInt();
                protocolConfig.IsAsync = iter2->value["Async"].GetBool();
                if (iter2->value.HasMember("Request"))
                {
                    const rapidjson::Value &jsonValue = iter2->value["Request"];
                    GKAssertRetFalse_F(jsonValue.IsString());
                    protocolConfig.Request = jsonValue.GetString();
                    if(MessagePool::New(protocolConfig.Request) == nullptr)
                    {
                        GKDebugFatal("create " << protocolConfig.Response << " failure");
                        return false;
                    }
                }

                if (iter2->value.HasMember("Response"))
                {
                    const rapidjson::Value &jsonValue = iter2->value["Response"];
                    GKAssertRetFalse_F(jsonValue.IsString());
                    protocolConfig.Response = jsonValue.GetString();
                    protocolConfig.Response = jsonValue["Message"].GetString();
                    if (MessagePool::New(protocolConfig.Response) == nullptr)
                    {
                        GKDebugFatal("create " << protocolConfig.Response << " failure");
                        return false;
                    }
                }

                methods.push_back(protocolConfig);
                std::string name = service + "." + protocolConfig.Method;

                this->mProtocolNameMap.emplace(name, protocolConfig);
                this->mProtocolIdMap.emplace(protocolConfig.MethodId, protocolConfig);
            }
            this->mServiceMap.emplace(service, methods);
        }
        return true;
    }

    const ProtocolConfig *RpcProtoComponent::GetProtocolConfig(int methodId) const
    {
        auto iter = this->mProtocolIdMap.find(methodId);
        return iter == this->mProtocolIdMap.end() ? &iter->second : nullptr;
    }

	bool RpcProtoComponent::HasService(const std::string & service)
	{
		auto iter = this->mServiceMap.find(service);
		return iter != this->mServiceMap.end();
	}

	void RpcProtoComponent::GetServices(std::vector<std::string> & services)
	{
		auto iter = this->mServiceMap.begin();
		for (; iter != this->mServiceMap.end(); iter++)
		{
			services.push_back(iter->first);
		}
	}

	bool RpcProtoComponent::GetMethods(const std::string & service, std::vector<std::string> & methods)
	{
		auto iter = this->mServiceMap.find(service);
		if (iter == this->mServiceMap.end())
		{
			return false;
		}
		for (const ProtocolConfig & config : iter->second)
		{
			methods.push_back(config.Method);
		}
		return true;
	}

    const ProtocolConfig * RpcProtoComponent::GetProtocolConfig(const std::string &fullName) const
    {
        auto iter = this->mProtocolNameMap.find(fullName);
        return iter != this->mProtocolNameMap.end() ? &iter->second : nullptr;
    }
}
