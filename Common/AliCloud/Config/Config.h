//
// Created by 64658 on 2025/2/8.
//

#ifndef APP_OSS_CONFIG_H
#define APP_OSS_CONFIG_H
#include <string>
#include <vector>
#include "Http/Client/Http.h"
#include "Yyjson/Object/JsonObject.h"
namespace oss
{
	struct Config : public json::Object<Config>
	{
		std::string key;
		std::string bucket;
		std::string region;
		std::string secret;
		std::string proto;
	public:
		inline std::string GetUrl() const {
			return fmt::format("{}://{}.{}.aliyuncs.com",
					this->proto, this->bucket, this->region);
		}
	};

	struct Policy
	{
		long long expiration;
		long long max_length;
		std::string file_type;
		std::string file_name;
		std::string upload_dir;
		std::vector<std::string> limit_type;
	};

	struct FromData
	{
		std::string policy;
		std::string OSSAccessKeyId;
		std::string signature;

		std::string fileName;
		std::string objectKey;

		std::string url;
		std::string host;
	};

	struct Response
	{
		std::string url;
		std::string data;
		HttpStatus code = HttpStatus::OK;
	};
}

#endif //APP_CONFIG_H
