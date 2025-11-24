//
// Created by leyi on 2024/4/1.
//

#include <ctime>
#include <iomanip>
#include "AliOssComponent.h"
#include "Entity/Actor/App.h"
#include "Util/Tools/TimeHelper.h"
#include "Util/Crypt/Base64Helper.h"
#include "Http/Client/Http.h"
#include "Util/File/FileHelper.h"
#include "Util/Tools/String.h"
#include "Http/Common/Content.h"
#include "Http/Common/HttpRequest.h"
#include "Http/Common/HttpResponse.h"
#include "Http/Component/HttpComponent.h"
#include "Util/Crypt/sha1.h"
#include "Lua/Lib/Lib.h"

namespace acs
{

	inline std::string get_utc_time() {
		std::time_t t = std::time(nullptr);
		std::tm* gmt = std::gmtime(&t);
		std::stringstream ss;
		ss << std::put_time(gmt, "%a, %d %b %Y %H:%M:%S GMT");
		return ss.str();
	}

	inline bool iso8601ToTimestamp(const std::string& iso8601, long long & target) {
		std::tm tm = {};
		std::istringstream ss(iso8601);
		ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
		if (ss.fail()) {
			return false;
		}
#ifdef __OS_WIN__
		target = (long long)_mkgmtime(&tm);
#else
		target = (long long)timegm(&tm);
#endif
		return true;
	}

	inline std::string create_signature(const std::string& method, const std::string& content_md5, const std::string& content_type,
			const std::string& date, const std::string& canonicalized_resource, const std::string& access_key_secret) {
		std::string string_to_sign = method + "\n" + content_md5 + "\n" + content_type + "\n" + date + "\n" + canonicalized_resource;
		std::string signature = help::Sha1::GetHMacHash(access_key_secret, string_to_sign);
		return help::Base64::Encode(signature);
	}


	AliOssComponent::AliOssComponent()
	{
		this->mHttp = nullptr;
		REGISTER_JSON_CLASS_FIELD(oss::Config, bucket);
		REGISTER_JSON_CLASS_FIELD(oss::Config, region);
		REGISTER_JSON_CLASS_MUST_FIELD(oss::Config, key);
		REGISTER_JSON_CLASS_MUST_FIELD(oss::Config, proto);
		REGISTER_JSON_CLASS_MUST_FIELD(oss::Config, secret);
	}

	bool AliOssComponent::Awake()
	{
		LuaCCModuleRegister::Add([](Lua::CCModule & ccModule) {
			ccModule.Open("ali.oss", lua::lib::luaopen_loss);
		});
		ServerConfig & config = this->mApp->GetConfig();
		LOG_CHECK_RET_FALSE(config.Get("ali.oss", this->mConfig))
		return true;
	}

	void AliOssComponent::OnStart()
	{
		oss::Config config;
		config.key = this->mConfig.key;
		config.secret = this->mConfig.secret;
		std::unique_ptr<http::Request> request = this->New("GET", config, "", false);
		{
#ifndef __ENABLE_OPEN_SSL__
			request->SetUrl("http://oss.aliyuncs.com");
#else
			request->SetUrl("https://oss.aliyuncs.com");
#endif
		}
		std::unique_ptr<http::Content> response = this->mHttp->Run(request);
		LOG_CHECK_RET(response != nullptr && response->GetContentType() == http::ContentType::XML);
		const http::XMLContent* xmlContent = response->Cast<http::XMLContent>();

		std::unique_ptr<xml::XElement> xElement;
		xmlContent->Get("Buckets", xElement);
		std::vector<std::unique_ptr<xml::XElement>> xElements;
		xElement->Get("Bucket", xElements);
		for(std::unique_ptr<xml::XElement> & element :xElements)
		{
			std::string name, location;
			element->Get("Name", name);
			element->Get("Location", location);
			this->mLocals.emplace_back(name, location);
		}
		std::vector<tinyxml2::XMLNode *> bucketsElement;
	}

	bool AliOssComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mHttp = this->GetComponent<HttpComponent>())
		return true;
	}

	bool AliOssComponent::GetLocation(const std::string& bucket, oss::Config & config)
	{
		auto iter = std::find_if(this->mLocals.begin(), this->mLocals.end(),
				[&bucket](const std::pair<std::string, std::string>& item)
				{
					return item.first == bucket;
				});
		if(iter == this->mLocals.end())
		{
			LOG_ERROR("not find oss bucket:{}", bucket);
			return false;
		}
		config.bucket = bucket;
		config.region = iter->second;
		config.key = this->mConfig.key;
		config.proto = this->mConfig.proto;
		config.secret = this->mConfig.secret;
		return true;
	}

	bool AliOssComponent::Delete(const std::string& objectKey)
	{
		return this->Delete(this->mConfig, objectKey);
	}

	bool AliOssComponent::Delete(const oss::Config& config, const std::string& objectKey)
	{
		const std::string date = get_utc_time();
		std::string url = fmt::format("{}/{}", config.GetUrl(), objectKey);
		std::unique_ptr<http::Request> httpRequest = this->New("DELETE", config, objectKey);
		std::unique_ptr<http::Response> httpResponse = this->Run(url, httpRequest);
		if(httpResponse == nullptr || httpResponse->Code() != HttpStatus::OK)
		{
			return false;
		}
		return httpResponse->Code() == HttpStatus::OK || httpResponse->Code() == HttpStatus::NO_CONTENT;
	}

	std::unique_ptr<oss::List> AliOssComponent::List(const std::string& dir, int count)
	{
		return this->List(this->mConfig, dir, count);
	}

	std::unique_ptr<http::Response> AliOssComponent::Run(const std::string& url, std::unique_ptr<http::Request>& request)
	{
		if(!request->SetUrl(url))
		{
			return nullptr;
		}
		std::unique_ptr<http::Response> httpResponse = this->mHttp->Do(request);
		if(httpResponse == nullptr)
		{
			return nullptr;
		}
		if(httpResponse->Code() != HttpStatus::OK)
		{
			xml::XDocument xDocument;
			const http::XMLContent * xmContent = httpResponse->To<http::XMLContent>();
			if(xmContent != nullptr)
			{
				std::string code, message;
				xmContent->Get("Code", code);
				xmContent->Get("Message", message);
				LOG_ERROR("[{}] {} => {}", url, code, message);
			}
		}
		return httpResponse;
	}

	std::unique_ptr<oss::List> AliOssComponent::List(const oss::Config& config, const std::string& dir, int count)
	{
		std::string args = fmt::format("?prefix={}/&max-keys={}", dir, count);
		const std::string url = fmt::format("{}/{}", config.GetUrl(), args);
		std::unique_ptr<http::Request> httpRequest = this->New("GET", config, "");
		std::unique_ptr<http::Response> httpResponse = this->Run(url, httpRequest);
		if(httpResponse == nullptr || httpResponse->Code() != HttpStatus::OK)
		{
			return nullptr;
		}
		const http::XMLContent * xmlContent = httpResponse->To<http::XMLContent>();
		if(xmlContent == nullptr)
		{
			return nullptr;
		}
		std::unique_ptr<oss::List> objectList = std::make_unique<oss::List>();
		{
			xmlContent->Get("Name", objectList->name);
			xmlContent->Get("Prefix", objectList->prefix);
			std::vector<std::unique_ptr<xml::XElement>> xElements;
			if (!xmlContent->Get("Contents", xElements))
			{
				return nullptr;
			}
			for (std::unique_ptr<xml::XElement>& element: xElements)
			{
				oss::Object ossObject;
				std::string writeTime;
				element->Get("Key", ossObject.key);
				element->Get("Size", ossObject.size);
				element->Get("LastModified", writeTime);
				iso8601ToTimestamp(writeTime, ossObject.lastWriteTime);
				objectList->contents.emplace_back(ossObject);
			}
		}
		return objectList;
	}

	bool AliOssComponent::Download(const std::string& objectKey, const std::string & path)
	{
		return this->Download(this->mConfig, objectKey, path);
	}

	bool AliOssComponent::Download(const oss::Config& config, const std::string& objectKey, const std::string & path)
	{
		std::unique_ptr<http::FileContent> fileContent = std::make_unique<http::FileContent>();
		if(!fileContent->MakeFile(path))
		{
			return false;
		}
		std::string url = fmt::format("{}/{}", config.GetUrl(), objectKey);
		std::unique_ptr<http::Request> httpRequest = this->New("GET", config, objectKey);
		{
			httpRequest->SetUrl(url);
		}
		std::unique_ptr<http::Response> response = this->mHttp->Do(httpRequest, std::move(fileContent));
		if(response == nullptr || response->Code() != HttpStatus::OK)
		{
			LOG_ERROR("{}", response->ToString())
			return false;
		}
		return true;
	}

	bool AliOssComponent::Download(const std::string& bucket, const std::string& objectKey, const std::string& path)
	{
		oss::Config ossConfig;
		if(!this->GetLocation(bucket, ossConfig))
		{
			return false;
		}
		return this->Download(ossConfig, objectKey, path);
	}

	std::unique_ptr<oss::Response> AliOssComponent::Upload(const std::string & bucket, const std::string& path, const std::string& objectKey)
	{
		oss::Config ossConfig;
		if(!this->GetLocation(bucket, ossConfig))
		{
			return nullptr;
		}
		return this->Upload(ossConfig, path, objectKey);
	}

	std::unique_ptr<http::Request> AliOssComponent::New(const char* method,
			const std::string& bucket, const std::string& objectKey, bool contentType)
	{
		oss::Config ossConfig;
		if(!this->GetLocation(bucket, ossConfig))
		{
			return nullptr;
		}
		return this->New(method, ossConfig, objectKey, contentType);
	}

	std::unique_ptr<http::Request> AliOssComponent::New(const char* method,
			const oss::Config& config, const std::string& objectKey, bool hasContentType)
	{
		std::unique_ptr<http::Request> httpRequest = std::make_unique<http::Request>(method);
		std::unique_ptr<oss::AuthInfo> authInfo = this->GenAuth(method, config, objectKey, hasContentType);
		{
			for(const std::pair<std::string, std::string> & item : authInfo->header)
			{
				httpRequest->Header().Add(item.first, item.second);
			}
		}
		return httpRequest;
	}

	std::unique_ptr<http::Request> AliOssComponent::New(const std::string& path, const std::string & objectKey)
	{
		return this->New(this->mConfig, path, objectKey);
	}

	std::unique_ptr<http::Request> AliOssComponent::New(
			const oss::Config& ossConfig, const std::string& path, const std::string& objectKey)
	{
		std::string fileName;
		if (!help::Str::GetFileName(path, fileName))
		{
			return nullptr;
		}
		std::string url = fmt::format("{}/{}", ossConfig.GetUrl(), objectKey);
		std::unique_ptr<http::Request> httpRequest = this->New("PUT", ossConfig, objectKey, true);
		{
			httpRequest->SetUrl(url);
		}
		std::unique_ptr<http::FileContent> fileContent = std::make_unique<http::FileContent>();
		{
			if (!fileContent->OpenFile(path))
			{
				LOG_ERROR("[open] => {}", path)
				return nullptr;
			}
		}
		httpRequest->SetContent(std::move(fileContent));
		return httpRequest;
	}

	std::unique_ptr<http::Request> AliOssComponent::New(const std::string& bucket,
			const std::string& path, const std::string& objectKey)
	{
		oss::Config ossConfig;
		if(!this->GetLocation(bucket, ossConfig))
		{
			return nullptr;
		}
		return this->New(ossConfig, path, objectKey);
	}

	std::unique_ptr<oss::Response> AliOssComponent::Upload(const std::string& path, const std::string& objectKey)
	{
		return this->Upload(this->mConfig, path, objectKey);
	}

	std::unique_ptr<oss::Response> AliOssComponent::Upload(const oss::Config & config, const std::string& path, const std::string & objectKey)
	{
		std::unique_ptr<oss::Response> ossResponse = std::make_unique<oss::Response>();
		do
		{
			if (!help::fs::FileIsExist(path))
			{
				ossResponse->data = "file not exist";
				ossResponse->code = HttpStatus::BAD_REQUEST;
				break;
			}
			std::string fileName;
			if (!help::Str::GetFileName(path, fileName))
			{
				ossResponse->data = "get file name fail";
				ossResponse->code = HttpStatus::BAD_REQUEST;
				break;
			}
			std::string url = fmt::format("{}/{}", config.GetUrl(), objectKey);
			std::unique_ptr<http::Request> httpRequest = this->New("PUT", config, objectKey, true);
			std::unique_ptr<http::FileContent> fileContent = std::make_unique<http::FileContent>();
			{
				if (!fileContent->OpenFile(path))
				{
					ossResponse->data = "open file fail";
					ossResponse->code = HttpStatus::BAD_REQUEST;
					break;
				}
			}
			httpRequest->SetContent(std::move(fileContent));
			std::unique_ptr<http::Response> response = this->Run(url, httpRequest);
			if (response == nullptr)
			{
				ossResponse->data = "response is null";
				ossResponse->code = HttpStatus::INTERNAL_SERVER_ERROR;
				break;
			}
			ossResponse->code = response->Code();
			if (response->Code() == HttpStatus::OK)
			{
				ossResponse->url = url;
				break;
			}
			if(response->GetBody() != nullptr)
			{
				ossResponse->data = response->GetBody()->ToStr();
			}
		}
		while (false);
		return ossResponse;
	}

	bool AliOssComponent::Sign(const oss::Policy& policy, const oss::Config & config, oss::FromData& fromData)
	{
		json::w::Document document1;
		document1.Add("expiration", help::Time::GetDateISO(policy.expiration));
		auto conditionArray = document1.AddArray("conditions");

		auto jsonArray1 = conditionArray->AddArray();
		{
			jsonArray1->Push("eq");
			jsonArray1->Push("$bucket");
			jsonArray1->Push(config.bucket);
		}
		auto jsonArray2 = conditionArray->AddArray();
		{
			jsonArray2->Push("eq");
			jsonArray2->Push("$success_action_status");
			jsonArray2->Push("200");
		}
		if (!policy.upload_dir.empty())
		{
			auto jsonUpload = conditionArray->AddArray();
			jsonUpload->Push("starts-with");
			jsonUpload->Push("$key");
			jsonUpload->Push(policy.upload_dir + '/');
		}
		auto jsonArray = conditionArray->AddArray();
		{
			jsonArray->Push("content-length-range");
			jsonArray->Push(0);
			jsonArray->Push(policy.max_length);
		}

		auto jsonArray3 = conditionArray->AddArray();
		{
			jsonArray3->Push("in");
			jsonArray3->Push("$content-type");
			auto jsonArray4 = jsonArray3->AddArray();
			for (const std::string& type: policy.limit_type)
			{
				jsonArray4->Push(type);
			}
//			if(!policy.file_type.empty())
//			{
//				jsonArray4->Push(policy.file_type);
//			}
		}

		std::string str1 = document1.JsonString();
		std::string str2 = help::Base64::Encode(str1);
		std::string str3 = help::Sha1::GetHMacHash(config.secret, str2);

		std::string host = config.GetUrl();
		size_t pos = policy.file_type.find('/');
		if(pos == std::string::npos)
		{
			return false;
		}
		std::string type = policy.file_type.substr(pos + 1);
		fromData.fileName = policy.file_name + '.' + type;
		std::string fullName = policy.upload_dir + '/' + fromData.fileName;

		fromData.policy = str2;
		fromData.objectKey = fullName;
		fromData.host = config.GetUrl();
		fromData.OSSAccessKeyId = config.key;
		fromData.signature = help::Base64::Encode(str3);
		fromData.url = fmt::format("{}/{}", host, fullName);
		return true;
	}

	bool AliOssComponent::Sign(const oss::Policy& policy, oss::FromData& fromData)
	{
		return this->Sign(policy, this->mConfig, fromData);
	}

	bool AliOssComponent::Sign(const oss::Policy& policy, const std::string& bucket, oss::FromData& fromData)
	{
		oss::Config config;
		if(!this->GetLocation(bucket, config))
		{
			return false;
		}
		return this->Sign(policy, config, fromData);
	}

	std::unique_ptr<oss::AuthInfo> AliOssComponent::GenAuth(
			const char* method, const std::string& objectKey, bool hasContentType)
	{
		return this->GenAuth(method, this->mConfig, objectKey, hasContentType);
	}

	std::unique_ptr<oss::AuthInfo> AliOssComponent::GenAuth(
			const char* method, const std::string& bucket, const std::string& objectKey, bool hasContentType)
	{
		oss::Config config;
		if(!this->GetLocation(bucket, config))
		{
			return nullptr;
		}
		return this->GenAuth(method, config, objectKey, hasContentType);
	}

	std::unique_ptr<oss::AuthInfo> AliOssComponent::GenAuth(
			const char * method, const oss::Config& config, const std::string & objectKey, bool hasContentType)
	{
		std::unique_ptr<oss::AuthInfo> result = std::make_unique<oss::AuthInfo>();
		std::string contentType, fileMd5;
		const std::string date = get_utc_time();
		if (hasContentType && !objectKey.empty())
		{
			std::string fileType;
			help::fs::GetFileType(objectKey, fileType);
			contentType = http::GetContentType(fileType);
		}
		std::string canonicalized_resource = "/";
		if (!config.bucket.empty())
		{
			canonicalized_resource.append(config.bucket);
		}
		if (!objectKey.empty())
		{
			canonicalized_resource += '/';
			canonicalized_resource.append(objectKey);
		}
		std::string sign = create_signature(method, fileMd5, contentType,
				date, canonicalized_resource, config.secret);
		if (!contentType.empty())
		{
			result->header.emplace_back(http::Header::ContentType, contentType);
		}

		std::string auth = fmt::format("OSS {}:{}", config.key, sign);
		result->header.emplace_back("Date", date);
		result->header.emplace_back("User-Agent", "acs");
		result->header.emplace_back(http::Header::Auth, auth);
		result->url = fmt::format("{}/{}", config.GetUrl(), objectKey);
		return result;
	}

	bool AliOssComponent::GetUrl(const std::string& objectKey, std::string & url)
	{
		return this->GetUrl(this->mConfig, objectKey, url);
	}

	bool AliOssComponent::GetUrl(const oss::Config& config, const std::string& objectKey, std::string& url)
	{
		url = fmt::format("{}/{}", config.GetUrl(), objectKey);
		return true;
	}
}