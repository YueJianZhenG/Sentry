//
// Created by 64658 on 2025/8/7.
//

#include "AliOss.h"
#include "Entity/Actor/App.h"
#include "AliCloud/Component/AliOssComponent.h"
namespace acs
{
	AliOss::AliOss()
	{
		this->mAliOss = nullptr;
	}

	bool AliOss::OnInit()
	{
		BIND_COMMON_HTTP_METHOD(AliOss::Url);
		BIND_COMMON_HTTP_METHOD(AliOss::Make);
		BIND_COMMON_HTTP_METHOD(AliOss::Upload);
		this->mAliOss = this->GetComponent<AliOssComponent>();
		return true;
	}

	int AliOss::Make(const http::FromContent& request, json::w::Document& response)
	{
		std::string type, objectKey;
		LOG_ERROR_CHECK_ARGS(request.Get("type", type))
		LOG_ERROR_CHECK_ARGS(request.Get("key", objectKey))

		size_t pos = type.find('/');
		if(pos == std::string::npos)
		{
			return XCode::CallArgsError;
		}
		std::string t = type.substr(pos + 1);
		std::unique_ptr<json::w::Value> resultObject = response.AddObject("header");
		std::unique_ptr<oss::AuthInfo> result = this->mAliOss->GenAuth("PUT", objectKey, true);
		for(const std::pair<std::string, std::string> & itemInfo : result->header)
		{
			resultObject->Add(itemInfo.first.c_str(), itemInfo.second);
		}
		response.Add("url", result->url);
		return XCode::Ok;
	}

	int AliOss::Upload(const http::Request& request, http::Response& response)
	{
		std::string contentType, name;
		const http::FromContent & query = request.GetUrl().GetQuery();
		LOG_ERROR_CHECK_ARGS(query.Get("name", name));
		if(!request.ConstHeader().GetContentType(contentType))
		{
			return XCode::CallArgsError;
		}
		const http::Content * content = request.GetBody();
		if(content == nullptr)
		{
			return XCode::Failure;
		}
		const http::FileContent * fileContent = content->To<const http::FileContent>();
		if(fileContent == nullptr)
		{
			return XCode::Failure;
		}
		size_t pos = contentType.find('/');
		if(pos == std::string::npos)
		{
			return XCode::Failure;
		}
		std::string id = this->mApp->NewUuid();
		std::string type = contentType.substr(pos + 1);
		const std::string & path = fileContent->Path();
		std::string objectKey = fmt::format("{}/{}.{}", name, id, type);
		std::unique_ptr<oss::Response> ossResponse = this->mAliOss->Upload(path, objectKey);
		if(ossResponse == nullptr || ossResponse->code != HttpStatus::OK)
		{
			return XCode::Failure;
		}
		std::remove(path.c_str());
		response.SetContent(http::Header::TEXT, ossResponse->url);
		return XCode::Ok;
	}

	int AliOss::Url(const http::FromContent& request, http::Response& response)
	{
		std::string url;
		std::string objectKey;
		LOG_ERROR_CHECK_ARGS(request.Get("key", objectKey))
		LOG_ERROR_CHECK_ARGS(this->mAliOss->GetUrl(objectKey, url));
		response.SetContent(http::Header::TEXT, url);
		return XCode::Ok;
	}
}