//
// Created by yy on 2023/12/24.
//

#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include "Yyjson/Object/JsonObject.h"
namespace http
{
	struct Config : public json::Object<Config>
	{
		bool auth;
		int pool = 0; //对象池数量
		std::string index;
		//std::string upload;
		std::string domain;
		int send_timeout = 5;
		int read_timeout = 5;
		std::vector<std::string> root;
        std::vector<std::string> whiteList;
		std::unordered_map<std::string, std::string> header;
	};
}
#endif //APP_CONFIG_H
