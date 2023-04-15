//
// Created by zmhy0073 on 2022/10/25.
//

#include"RedisConfig.h"
#include"Util/String/StringHelper.h"
#include"rapidjson/document.h"
namespace Tendo
{
    bool RedisConfig::OnLoadText(const char *str, size_t length)
	{
		rapidjson::Document document;
		if (document.Parse(str, length).HasParseError())
		{
			return false;
		}
		if (!document.HasMember("redis"))
		{
			return false;
		}
		const rapidjson::Value& jsonData = document["redis"];
		{
			this->mConfig.FreeClient = 30;
			this->mConfig.Index = jsonData["index"].GetInt();
			this->mConfig.Count = jsonData["count"].GetInt();
		}
        if(!jsonData.HasMember("address"))
        {
            return false;
        }
		const rapidjson::Value & json = jsonData["address"];
        for(unsigned int index = 0;index < json.Size();index++)
        {
			std::string net;
            Net::Address addressInfo;
			addressInfo.FullAddress.assign(json[index].GetString());
            if(!Helper::Str::SplitAddr(addressInfo.FullAddress,net, addressInfo.Ip, addressInfo.Port))
            {
                return false;
            }
            this->mConfig.Address.emplace_back(addressInfo);
        }
		if (jsonData.HasMember("free"))
		{
			this->mConfig.FreeClient = jsonData["free"].GetInt();
		}
		if (jsonData.HasMember("passwd"))
		{
			this->mConfig.Password = jsonData["passwd"].GetString();
		}
		if (jsonData.HasMember("script"))
		{
			this->mConfig.Script = jsonData["script"].GetString();
			this->mConfig.Script = this->WorkPath() + this->mConfig.Script;
		}
		return true;
	}
}