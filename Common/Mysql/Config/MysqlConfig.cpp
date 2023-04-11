//
// Created by zmhy0073 on 2022/10/26.
//
#ifdef __ENABLE_MYSQL__


#include"MysqlConfig.h"
#include"Util/String/StringHelper.h"
#include"rapidjson/document.h"
namespace Tendo
{
    bool MysqlConfig::OnLoadText(const char *str, size_t length)
    {
        rapidjson::Document document;
        if (document.Parse(str, length).HasParseError())
        {
            return false;
        }
        if (!document.HasMember("mysql"))
        {
            return false;
        }
        rapidjson::Value &json = document["mysql"];
        this->User = json["user"].GetString();
        this->MaxCount = json["count"].GetInt();
        this->Password = json["passwd"].GetString();
        if (json.HasMember("ping"))
        {
            this->Ping = json["ping"].GetInt();
        }
        for (unsigned int index = 0; index < json["address"].Size(); index++)
        {
            Net::Address addressInfo;
            addressInfo.FullAddress.assign(json["address"][index].GetString());
            if(!Helper::Str::SplitAddr(addressInfo.FullAddress, addressInfo.Ip, addressInfo.Port))
            {
                return false;
            }
            this->Address.emplace_back(addressInfo);
        }
        return this->Address.size() > 0;
    }

    bool MysqlConfig::OnReloadText(const char *str, size_t length)
    {
        return true;
    }
}

#endif