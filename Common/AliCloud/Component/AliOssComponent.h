//
// Created by leyi on 2024/4/1.
//

#ifndef APP_ALIOSSCOMPONENT_H
#define APP_ALIOSSCOMPONENT_H

#include"Http/Client/Http.h"
#include "Http/Common/HttpRequest.h"
#include "Entity/Component/Component.h"

#include "AliCloud/Config/Config.h"


namespace oss
{
	struct Object
	{
		int size = 0;
		std::string key;
		long long lastWriteTime;
	};
	struct List
	{
		std::string name;
		std::string prefix;
		std::vector<Object> contents;
	};

	struct AuthInfo
	{
		std::string url;
		std::list<std::pair<std::string, std::string>> header;
	};
}

namespace http
{
	class Response;
}

namespace acs
{
	class AliOssComponent final : public Component, public IStart
	{
	public:
		AliOssComponent();
		~AliOssComponent() final = default;
	private:
		bool Awake() final;
		void OnStart() final;
		bool LateAwake() final;
	public:
		bool Delete(const std::string & objectKey);
		bool Delete(const oss::Config& config, const std::string & objectKey);
	public:
		std::unique_ptr<oss::List> List(const std::string & dir, int count = 100);
		std::unique_ptr<oss::List> List(const oss::Config& config, const std::string & dir, int count = 100);
	public:
		bool Download(const std::string & objectKey, const std::string & path);
		bool Download(const std::string &bucket, const std::string & objectKey, const std::string & path);
		bool Download(const oss::Config& config, const std::string & objectKey, const std::string & path);
	public:
		std::unique_ptr<http::Request> New(const std::string& path, const std::string & objectKey);
		std::unique_ptr<http::Request> New(const std::string & bucket, const std::string& path, const std::string & objectKey);
		std::unique_ptr<http::Request> New(const oss::Config& config, const std::string& path, const std::string & objectKey);
		std::unique_ptr<http::Request> New(const char * method, const std::string & bucket, const std::string& objectKey, bool contentType = false);
		std::unique_ptr<http::Request> New(const char * method, const oss::Config & config, const std::string& objectKey, bool contentType = false);
	public:
		std::unique_ptr<oss::Response> Upload(const std::string& path, const std::string & objectKey);
		std::unique_ptr<oss::Response> Upload(const oss::Config& config, const std::string& path, const std::string & objectKey);
		std::unique_ptr<oss::Response> Upload(const std::string & bucket, const std::string& path, const std::string & objectKey);
	private:
		bool GetLocation(const std::string & bucket, oss::Config & config);
		std::unique_ptr<http::Response> Run(const std::string & url, std::unique_ptr<http::Request>& request);
	public:
		bool Sign(const oss::Policy& policy, oss::FromData & fromData);
		bool Sign(const oss::Policy& policy, const std::string & bucket, oss::FromData & fromData);
		bool Sign(const oss::Policy& policy, const oss::Config & config, oss::FromData & fromData);
	public:
		bool GetUrl(const std::string & objectKey, std::string & url);
		bool GetUrl(const oss::Config & config, const std::string & objectKey, std::string & url);
		std::unique_ptr<oss::AuthInfo> GenAuth(const char * method, const std::string & objectKey, bool hasContentType = true);
		std::unique_ptr<oss::AuthInfo> GenAuth(const char * method, const std::string& bucket, const std::string & objectKey, bool hasContentType = true);
		std::unique_ptr<oss::AuthInfo> GenAuth(const char * method, const oss::Config& config, const std::string & objectKey, bool hasContentType = true);
	private:
		oss::Config mConfig;
		class HttpComponent* mHttp;
		std::vector<std::pair<std::string, std::string>> mLocals;
	};
}


#endif
