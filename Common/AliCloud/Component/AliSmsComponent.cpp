//
// Created by yy on 2025/11/20.
//

#include <iomanip>
#include <random>
#include "Lua/Lib/Lib.h"
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include "AliSmsComponent.h"
#include "Entity/Actor/App.h"
#include "Http/Common/HttpRequest.h"
#include "Http/Common/HttpResponse.h"
#include "Server/Config/ServerConfig.h"
#include "Http/Component/HttpComponent.h"
#include "Util/Tools/String.h"
#include "Util/Tools/TimeHelper.h"

constexpr const char* METHOD = "POST";
const std::string API_VERSION = "2017-05-25";
const std::string canonical_uri = "/";
const std::string signature_alg = "ACS3-HMAC-SHA256";
const std::string ENDPOINT = "dysmsapi.aliyuncs.com";

namespace ali
{

	std::string percent_code(const std::string& encode_str)
	{
		std::ostringstream oss;
		oss << std::hex << std::uppercase;
		for (unsigned char c : encode_str)
		{
			if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			{
				oss << c;
			}
			else
			{
				oss << "%" << std::setw(2) << std::setfill('0') << static_cast<int>(c);
			}
		}

		return oss.str();
	}

	// 工具函数：SHA256哈希（返回小写十六进制）
	std::string sha256_hex(const std::string& input)
	{
		unsigned char hash[SHA256_DIGEST_LENGTH];
		SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

		std::ostringstream oss;
		oss << std::hex << std::nouppercase;
		for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
		{
			oss << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
		}
		return oss.str();
	}

	// 工具函数：HMAC-SHA256签名（返回二进制数据）
	std::vector<unsigned char> hmac256(const std::string& key, const std::string& input)
	{
		unsigned int hash_len = 0;
		unsigned char hash[EVP_MAX_MD_SIZE] = { 0 };
		HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
		     reinterpret_cast<const unsigned char*>(input.data()), static_cast<int>(input.size()),
		     hash, &hash_len);

		return std::vector<unsigned char>(hash, hash + hash_len);
	}

	// 工具函数：二进制转十六进制字符串（小写）
	std::string hex_encode(const std::vector<unsigned char>& data)
	{
		std::ostringstream oss;
		oss << std::hex << std::nouppercase;
		for (unsigned char c : data)
		{
			oss << std::setw(2) << std::setfill('0') << static_cast<int>(c);
		}
		return oss.str();
	}
}

namespace acs
{
	AliSmsComponent::AliSmsComponent()
	{
		this->mHttp = nullptr;
		REGISTER_JSON_CLASS_MUST_FIELD(ali::SmsConfig, signName);
		REGISTER_JSON_CLASS_MUST_FIELD(ali::SmsConfig, templateID);
		REGISTER_JSON_CLASS_MUST_FIELD(ali::SmsConfig, AccessKeyID);
		REGISTER_JSON_CLASS_MUST_FIELD(ali::SmsConfig, AccessSecret);
	}

	bool AliSmsComponent::Awake()
	{
		ServerConfig& config = this->mApp->GetConfig();
		LOG_CHECK_RET_FALSE(config.Get("ali.sms", this->mConfig))
		LuaCCModuleRegister::Add([](Lua::CCModule& ccModule)
		{
			ccModule.Open("ali.sms", lua::lib::luaopen_lsms);
		});
		return true;
	}

	bool AliSmsComponent::LateAwake()
	{
		this->mHttp = this->GetComponent<HttpComponent>();
		return true;
	}

	std::string AliSmsComponent::BuildCanonicalRequest(
		const std::vector<std::pair<std::string, std::string>>& query_params)
	{
		std::vector<std::pair<std::string, std::string>> sorted_params = query_params;
		std::sort(sorted_params.begin(), sorted_params.end(),
		          [](const auto& a, const auto& b) { return a.first < b.first; });

		std::ostringstream oss;
		for (size_t i = 0; i < sorted_params.size(); ++i)
		{
			if (i > 0)
			{
				oss << "&";
			}
			const std::string & key = sorted_params[i].first;
			const std::string & value = sorted_params[i].second;
			oss << ali::percent_code(key) << "=" << ali::percent_code(value);
		}
		return oss.str();
	}

	bool AliSmsComponent::SendCode(const std::string& phoneNum, const std::string& code)
	{
		return this->SendCode(this->mConfig.templateID, phoneNum, code);
	}

	bool AliSmsComponent::SendCode(const std::string& templateId,
		const std::string& phoneNum, const std::string& code)
	{
		json::w::Document document;
		document.Add("code", code);
		std::string parameter = document.JsonString();
		std::unique_ptr<http::Request> httpRequest = this->MakeRequest(templateId, phoneNum, parameter);
		{
			std::unique_ptr<http::Response> httpResponse = this->mHttp->Do(httpRequest);
			if (httpResponse == nullptr || httpResponse->Code() != HttpStatus::OK)
			{
				return false;
			}
			LOG_INFO("Ali SMS success: {}", httpResponse->ToString())
		}
		return true;
	}

	std::unique_ptr<http::Request> AliSmsComponent::MakeRequest(const std::string& phoneNum, const std::string& paramater)
	{
		return this->MakeRequest(this->mConfig.templateID, phoneNum, paramater);
	}

	std::unique_ptr<http::Request> AliSmsComponent::MakeRequest(const std::string& templateId, const std::string& phoneNum, const json::w::Document& parame)
	{
		std::string paramater;
		parame.Serialize(&paramater);
		return this->MakeRequest(templateId, phoneNum, paramater);
	}

	std::unique_ptr<http::Request> AliSmsComponent::MakeRequest(
		const std::string& templateId, const std::string& phoneNum, const std::string& templateParam)
	{
		std::vector<std::pair<std::string, std::string>> body_params = {
			{"PhoneNumbers", phoneNum},
			{"SignName", this->mConfig.signName},
			{"TemplateCode", templateId},
			{"TemplateParam", templateParam}
		};
		std::string queryString;
		std::string request_body = BuildCanonicalRequest(body_params);

		// 2. 构造请求头核心字段
		std::string nonce = help::Str::RandomString(32);
		std::string timestamp = help::Time::GetGMTDateString();
		std::string hashed_payload = ali::sha256_hex(request_body);

		// 3. 构造签名所需的 Headers 集合（key 已为小写）
		std::map<std::string, std::string> headers_to_sign;
		{
			headers_to_sign["content-length"] = std::to_string(request_body.size()); // 字节长度
			headers_to_sign["content-type"] = http::Header::FORM;
			headers_to_sign["host"] = ENDPOINT;
			headers_to_sign["x-acs-action"] = "SendSms";
			headers_to_sign["x-acs-content-sha256"] = hashed_payload;
			headers_to_sign["x-acs-date"] = timestamp;
			headers_to_sign["x-acs-signature-nonce"] = nonce;
			headers_to_sign["x-acs-version"] = API_VERSION;
		}

		bool first = true;
		std::string signed_headers;
		std::ostringstream canonical_headers_oss;
		for (const auto& pair : headers_to_sign)
		{
			std::string value = pair.second;
			auto lpos = value.find_first_not_of(" \t\r\n");
			auto rpos = value.find_last_not_of(" \t\r\n");
			std::string trimmed = (lpos == std::string::npos) ? "" : value.substr(lpos, rpos - lpos + 1);

			canonical_headers_oss << pair.first << ":" << trimmed << "\n";

			if (!first)
			{
				signed_headers += ';';
			}
			first = false;
			signed_headers += pair.first;
		}
		std::string canonical_headers = canonical_headers_oss.str();

		std::string canonical_request = fmt::format(
			"{}\n{}\n{}\n{}\n{}\n{}", METHOD, canonical_uri, queryString, canonical_headers, signed_headers, hashed_payload
		);

		std::string hashed_canonical_request = ali::sha256_hex(canonical_request);
		std::string string_to_sign = fmt::format("{}\n{}", signature_alg, hashed_canonical_request);
		std::vector<unsigned char> signature_bin = ali::hmac256(this->mConfig.AccessSecret, string_to_sign);

		std::string signature = ali::hex_encode(signature_bin);
		std::string authorization = fmt::format("{} Credential={},SignedHeaders={},Signature={}",
		                                        signature_alg,
		                                        this->mConfig.AccessKeyID,
		                                        signed_headers,
		                                        signature);

		std::unique_ptr<http::Request> httpRequest = std::make_unique<http::Request>(METHOD);
		httpRequest->SetUrl(fmt::format("https://{}{}{}", ENDPOINT, canonical_uri, queryString));
		std::unique_ptr<http::StringContent> httpContent = std::make_unique<http::StringContent>(request_body);
		{
			http::Head& head = httpRequest->Header();
			for (const auto& pair : headers_to_sign)
			{
				head.Add(pair.first, pair.second);
			}
			head.Add(http::Header::Auth, authorization);
			httpRequest->SetContent(std::move(httpContent));
		}
		return httpRequest;
	}
}
