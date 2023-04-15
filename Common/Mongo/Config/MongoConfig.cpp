//
// Created by zmhy0073 on 2022/10/26.
//

#include"MongoConfig.h"
#include"rapidjson/document.h"
#include"Util/String/StringHelper.h"
namespace Mongo
{
    bool MongoConfig::OnLoadText(const char *str, size_t length)
	{
		rapidjson::Document document;
		if (document.Parse(str, length).HasParseError())
		{
			return false;
		}
		if (!document.HasMember("mongo"))
		{
			return false;
		}
		const rapidjson::Value& jsonData = document["mongo"];
		{
			this->DB = jsonData["db"].GetString();
			this->User = jsonData["user"].GetString();
			this->MaxCount = jsonData["count"].GetInt();
			this->Password = jsonData["passwd"].GetString();
		}
		const rapidjson::Value& jsonArray = jsonData["address"];
		for (unsigned int index = 0; index < jsonArray.Size(); index++)
		{
			std::string net;
			Net::Address addressInfo;
			addressInfo.FullAddress.assign(jsonArray[index].GetString());
			if(!Helper::Str::SplitAddr(addressInfo.FullAddress, net, addressInfo.Ip, addressInfo.Port))
			{
				return false;
			}
			this->Address.emplace_back(addressInfo);
		}
		return this->Address.size() > 0;
	}

    bool MongoConfig::OnReloadText(const char *str, size_t length)
    {
        return true;
    }
}