//
// Created by leyi on 2024/3/28.
//

#include "FileUpload.h"
#include "Entity/Actor/App.h"
#include "Util/File/FileHelper.h"
#include "Util/Tools/TimeHelper.h"
#include "AliCloud/Component/AliOssComponent.h"
namespace acs
{
	FileUpload::FileUpload()
	{
		this->mOss = nullptr;
	}

	bool FileUpload::Awake()
	{
		json::r::Value jsonObject;
		if (!this->mApp->Config().Get("http", jsonObject))
		{
			return false;
		}
		return jsonObject.Get("domain", this->mDoMain);
	}

	bool FileUpload::OnInit()
	{
		BIND_COMMON_HTTP_METHOD(FileUpload::Oss);
		BIND_COMMON_HTTP_METHOD(FileUpload::File);
		BIND_COMMON_HTTP_METHOD(FileUpload::Proxy);
		this->mOss = this->GetComponent<AliOssComponent>();
		return true;
	}


	int FileUpload::Oss(const http::FromContent& request, json::w::Document & response)
	{
		oss::Policy policy;
		LOG_ERROR_CHECK_ARGS(request.Get("dir", policy.upload_dir))
		LOG_ERROR_CHECK_ARGS(request.Get("name", policy.file_name))
		LOG_ERROR_CHECK_ARGS(request.Get("type", policy.file_type))
		{
			long long nowTime = help::Time::NowSec();
			policy.expiration = nowTime + 30;
			policy.max_length = 1024 * 1024 * 5;
			policy.limit_type.emplace_back("image/png");
			policy.limit_type.emplace_back("image/jpg");
			policy.limit_type.emplace_back("image/jpeg");
			policy.limit_type.emplace_back("application/json");
		}
		oss::FromData ossFromData;
		this->mOss->Sign(policy, ossFromData);
		std::unique_ptr<json::w::Value> jsonValue = response.AddObject("data");
		{
			jsonValue->Add("name", ossFromData.fileName);
			jsonValue->Add("policy", ossFromData.policy);
			jsonValue->Add("OSSAccessKeyId", ossFromData.OSSAccessKeyId);
			jsonValue->Add("success_action_status", "200");
			jsonValue->Add("signature", ossFromData.signature);
			jsonValue->Add("key", ossFromData.objectKey);

			jsonValue->Add("url", ossFromData.url);
			jsonValue->Add("host", ossFromData.host);
		}
		return XCode::Ok;
	}

	int FileUpload::File(const http::Request &request, http::Response &response)
	{
		int userId = 0;
		const http::Content* data = request.GetBody();
		request.GetUrl().GetQuery().Get(http::query::UserId, userId);
		const http::MultipartFromContent* multiData = data->To<const http::MultipartFromContent>();
		if (multiData == nullptr)
		{
			return XCode::CallArgsError;
		}
		if (!multiData->IsDone())
		{
			return XCode::CallArgsError;
		}
		const std::string & path = multiData->Path();
		const std::string& name = multiData->FileName();
		const std::string url = fmt::format("{}/{}", this->mDoMain, name);
		response.SetContent(http::Header::TEXT, url);
		return XCode::Ok;
	}

	int FileUpload::Proxy(const http::Request& request, http::Response& response)
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
		std::unique_ptr<oss::Response> ossResponse = this->mOss->Upload(path, objectKey);
		if(ossResponse == nullptr || ossResponse->code != HttpStatus::OK)
		{
			return XCode::Failure;
		}
		std::remove(path.c_str());
		response.SetContent(http::Header::TEXT, ossResponse->url);
		return XCode::Ok;
	}
}