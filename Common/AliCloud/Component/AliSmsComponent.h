
#pragma once
#include "Http/Common/HttpRequest.h"
#include "Yyjson/Object/JsonObject.h"
#include "Entity/Component/Component.h"


namespace ali
{
	struct SmsConfig : public json::Object<SmsConfig>
	{
	public:
		std::string signName;
		std::string templateID;
		std::string AccessKeyID;
		std::string AccessSecret;
	};
}

namespace acs
{
	class AliSmsComponent : public Component
	{
	public:
		AliSmsComponent();
	private:
		bool Awake() final;
		bool LateAwake() final;
	public:
		bool SendCode(const std::string & phoneNum, const std::string & code);
		bool SendCode(const std::string & templateId, const std::string & phoneNum, const std::string & code);
		std::unique_ptr<http::Request> MakeRequest(const std::string & phoneNum, const std::string & parame);
		std::unique_ptr<http::Request> MakeRequest(const std::string & templateId, const std::string & phoneNum, const std::string & parame);
		std::unique_ptr<http::Request> MakeRequest(const std::string & templateId, const std::string & phoneNum, const json::w::Document & parame);
	private:
		static std::string BuildCanonicalRequest(const std::vector<std::pair<std::string, std::string>>& query_params);
	private:
		ali::SmsConfig mConfig;
		class HttpComponent * mHttp;
	};
}





