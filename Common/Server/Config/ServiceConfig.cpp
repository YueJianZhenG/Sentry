﻿#include "ServiceConfig.h"

#include "App/App.h"
#include "rapidjson/document.h"

namespace Sentry
{
	bool RpcServiceConfig::OnLoadConfig(const rapidjson::Value & json)
	{
		if(!json.IsObject() || !json.HasMember("Server"))
		{
			return false;
		}
		this->mConfigs.clear();
        this->mServer = json["Server"].GetString();
		auto iter = json.MemberBegin();
		for(; iter != json.MemberEnd(); iter++)
		{
			const rapidjson::Value & jsonValue = iter->value;
			if(jsonValue.IsObject())
			{
				RpcMethodConfig serviceConfog;
				const char* name = iter->name.GetString();

				serviceConfog.Method = name;
				serviceConfog.Service = this->mName;
				serviceConfog.Type = jsonValue["Type"].GetString();
				serviceConfog.IsAsync = jsonValue["Async"].GetBool();
				if (jsonValue.HasMember("Request"))
				{
					serviceConfog.Request = jsonValue["Request"].GetString();
				}
				if (jsonValue.HasMember("Response"))
				{
					serviceConfog.Response = jsonValue["Response"].GetString();
				}
				serviceConfog.FullName = fmt::format("{0}.{1}", this->mName, name);
				this->mConfigs.emplace(name, serviceConfog);
			}
		}
		return true;
	}
	bool RpcServiceConfig::ParseFunName(const std::string& func, std::string& service, std::string& method)
	{
		size_t pos = func.find('.');
		if(pos == std::string::npos)
		{
			return false;
		}
		service = func.substr(0, pos);
		method = func.substr(pos + 1);
		return true;
	}
}

namespace Sentry
{
	bool HttpServiceConfig::OnLoadConfig(const rapidjson::Value & json)
	{
		if(!json.IsObject())
		{
			return false;
		}
        this->mServer = json["Server"].GetString();
        auto iter = json.MemberBegin();
		for(; iter != json.MemberEnd(); iter++)
		{
			const rapidjson::Value & jsonValue = iter->value;
			if(jsonValue.IsObject())
			{
				HttpMethodConfig serviceConfog;
				const char* name = iter->name.GetString();

				serviceConfog.Method = name;
				serviceConfog.Service = this->mName;
				serviceConfog.Type = jsonValue["Type"].GetString();
				serviceConfog.Path = jsonValue["Path"].GetString();
				serviceConfog.IsAsync = jsonValue["Async"].GetBool();
				if (jsonValue.HasMember("Content"))
				{
					serviceConfog.Content = jsonValue["Content"].GetString();
				}
				this->mConfigs.emplace(serviceConfog.Method, serviceConfog);
			}
		}
		return true;
	}
}
