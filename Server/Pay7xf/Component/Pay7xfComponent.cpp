//
// Created by yy on 2025/9/29.
//

#include "Pay7xfComponent.h"
#include "Entity/Actor/App.h"
#include "Http/Common/HttpRequest.h"
#include "Util/Crypt/MD5Helper.h"
#include "Http/Component/HttpComponent.h"
#include "Util/Crypt/Base64Helper.h"
#include "Util/Crypt/Sign.h"
#include "Util/Ssl/SslCert.h"
#include "Util/Tools/TimeHelper.h"
#include "Util/Tools/String.h"

namespace acs
{
	inline std::string Base64ToPem(const std::string& base64_str, const std::array<std::string, 2>& list)
	{
		std::string pem_str = list[0] + "\n";
		for (size_t i = 0; i < base64_str.size(); i += 64)
		{
			pem_str += base64_str.substr(i, 64) + "\n";
		}
		pem_str += list[1] + "\n";
		return pem_str;
	}

	Pay7xfComponent::Pay7xfComponent()
	{
		this->mConfig.pid = 0;
		this->mHttp = nullptr;
		this->mApiVersion = pay_7xf::API_V2;
		this->mSignType = pay_7xf::sign::rsa;
		REGISTER_JSON_CLASS_MUST_FIELD(pay_7xf::Config, pid);
		REGISTER_JSON_CLASS_MUST_FIELD(pay_7xf::Config, md5);
		REGISTER_JSON_CLASS_MUST_FIELD(pay_7xf::Config, public_key);
		REGISTER_JSON_CLASS_MUST_FIELD(pay_7xf::Config, return_url);
		REGISTER_JSON_CLASS_MUST_FIELD(pay_7xf::Config, notify_url);
		REGISTER_JSON_CLASS_MUST_FIELD(pay_7xf::Config, private_key);

	}

	bool Pay7xfComponent::Awake()
	{
		json::r::Value jsonValue;
		const ServerConfig& config = this->mApp->Config();
		LOG_CHECK_RET_FALSE(config.Get("pay_7xf", jsonValue));
		LOG_CHECK_RET_FALSE(this->mConfig.Decode(jsonValue));
		LOG_CHECK_RET_FALSE(this->InitPublicKey());
		LOG_CHECK_RET_FALSE(this->InitPrivateKey());
		return true;
	}

	bool Pay7xfComponent::InitPrivateKey()
	{
		std::string pem_footer = "-----END PRIVATE KEY-----";
		std::string pem_header = "-----BEGIN PRIVATE KEY-----";

		std::array<std::string, 2> pem{
			"-----END PRIVATE KEY-----",
			"-----BEGIN PRIVATE KEY-----"
		};

		std::string base64_str = this->mConfig.private_key;
		std::string pemString = Base64ToPem(base64_str, pem);
		if (!help::ssl::IsPrivateKey(pemString))
		{
			std::array<std::string, 2> rsaPem{
				"-----BEGIN RSA PRIVATE KEY-----",
				"-----END RSA PRIVATE KEY-----"
			};
			pemString = Base64ToPem(base64_str, rsaPem);
			if (!help::ssl::IsPrivateKey(pemString))
			{
				return false;
			}
		}
		this->mConfig.private_key = pemString;
		return true;
	}

	bool Pay7xfComponent::InitPublicKey()
	{
		std::string pem_footer = "-----END PUBLIC KEY-----";
		std::string pem_header = "-----BEGIN PUBLIC KEY-----";

		std::array<std::string, 2> pem{
			"-----END PUBLIC KEY-----",
			"-----BEGIN PUBLIC KEY-----"
		};

		std::string base64_str = this->mConfig.public_key;
		std::string pemString = Base64ToPem(base64_str, pem);
		if (!help::ssl::IsPublicKey(pemString))
		{
			std::array<std::string, 2> rsaPem{
				"-----BEGIN RSA PRIVATE KEY-----",
				"-----END RSA PRIVATE KEY-----"
			};
			pemString = Base64ToPem(base64_str, rsaPem);
			if (!help::ssl::IsPublicKey(pemString))
			{
				return false;
			}
		}
		this->mConfig.public_key = pemString;
		return true;
	}

	bool Pay7xfComponent::LateAwake()
	{
		this->mHttp = this->GetComponent<HttpComponent>();
		return true;
	}

	bool Pay7xfComponent::UpdateProduct(int id, int money, const std::string& name)
	{
		if(id <= 0 || money <= 0 || name.empty())
		{
			return false;
		}
		auto iter = this->mProducts.find(id);
		if(iter != this->mProducts.end())
		{
			iter->second.name = name;
			iter->second.money = fmt::format("{:.2f}", (float)money / 100.0f);
			return true;
		}
		pay_7xf::ProductInfo productInfo;
		{
			productInfo.id = id;
			productInfo.name = name;
			productInfo.money = fmt::format("{:.2f}", (float)money / 100.0f);
		}
		this->mProducts.emplace(id, productInfo);
		return true;
	}

	bool Pay7xfComponent::SignParams(const std::map<std::string, std::string>& params, std::string & sign) const
	{
		std::string paramStr;
		for (auto iter = params.begin(); iter != params.end(); iter++)
		{
			if (iter->second.empty() || iter->first == "sign" || iter->first == "sign_type")
			{
				continue;
			}
			if (!paramStr.empty())
			{
				paramStr += "&";
			}
			paramStr += iter->first + "=" + iter->second;
		}
		auto iter = params.find("sign_type");
		if(iter == params.end())
		{
			return false;
		}
		bool result = true;
		const std::string& key = this->mConfig.private_key;
		do
		{
			if(iter->second == pay_7xf::sign::rsa)
			{
				if (!help::sign::Sha256WithRSA(paramStr, key, sign))
				{
					result = false;
					break;
				}
			}
			else if(iter->second == pay_7xf::sign::md5)
			{
				result = true;
				std::string str = paramStr + this->mConfig.md5;
				sign = help::md5::GetHex(str);
			}
		}
		while(false);
		return result;
	}

	const pay_7xf::ProductInfo* Pay7xfComponent::GetProduct(int productId)
	{
		auto iter = this->mProducts.find(productId);
		if(iter == this->mProducts.end())
		{
			return nullptr;
		}
		return &iter->second;
	}


	std::unique_ptr<pay_7xf::CreateResponse> Pay7xfComponent::Create(int productId, const std::string& ip)
	{
		const pay_7xf::ProductInfo* productInfo = this->GetProduct(productId);
		if(productInfo == nullptr)
		{
			return nullptr;
		}
		pay_7xf::CreateRequest createRequest;
		{
			createRequest.clientip = ip;
			createRequest.name = productInfo->name;
			createRequest.money = productInfo->money;
		}
		return this->Create(createRequest);
	}

	std::unique_ptr<pay_7xf::CreateResponse> Pay7xfComponent::Create(pay_7xf::CreateRequest& request)
	{
		request.pid = this->mConfig.pid;
		request.notify_url = this->mConfig.notify_url;
		request.return_url = this->mConfig.return_url;
		request.out_trade_no = std::to_string(help::ID::Gen());

		std::map<std::string, std::string> params
		{
				{"pid", std::to_string(request.pid)},
				//{"device", request.device},
				{"type", request.type},
				{"out_trade_no", request.out_trade_no},
				{"notify_url", request.notify_url},
				{"return_url", request.return_url},
				{"name", request.name},
				{"money", request.money},
				{"sitename", "chuanqi"},
				{"clientip", request.clientip}
		};
		std::unique_ptr<pay_7xf::CreateResponse> createResponse = std::make_unique<pay_7xf::CreateResponse>();
		{
			if(this->mApiVersion == pay_7xf::API_V1)
			{
				const std::string path = "submit.php";
				if(!this->Do(path, params, &createResponse->pay_info))
				{
					return nullptr;
				}
				char buffer[256] = { 0 };
				const char * str = createResponse->pay_info.c_str();
				if(!help::Str::Scanf(str, "%*[^']'%[^']'", buffer))
				{
					return nullptr;
				}
				createResponse->pay_info.assign(fmt::format("{}{}", pay_7xf::host, buffer));
				return createResponse;
			}
			const std::string path = "api/pay/create";
			params.emplace("method", request.method);
			params.emplace("timestamp", std::to_string((help::Time::NowSec())));
			std::unique_ptr<json::r::Document> document = std::make_unique<json::r::Document>();
			if(!this->Do(path, params, document.get()))
			{
				return nullptr;
			}

			document->Get("code", createResponse->code);
			if(createResponse->code != 0)
			{
				LOG_ERROR("{}", document->ToString());
				return nullptr;
			}
			document->Get("trade_no", createResponse->trade_no);
			document->Get("pay_info", createResponse->pay_info);
			document->Get("pay_type", createResponse->pay_type);
		}
		return createResponse;
	}

	bool Pay7xfComponent::Do(const std::string& path, std::map<std::string, std::string>& params, std::string * result)
	{
		std::string sign;
		params.emplace("sign_type", this->mSignType);
		if(!this->SignParams(params, sign))
		{
			return false;
		}
		params.emplace("sign", sign);
		std::unique_ptr<http::Request> httpRequest = std::make_unique<http::Request>("POST");
		std::unique_ptr<http::FromContent> httpFromContent = std::make_unique<http::FromContent>();
		{
			httpRequest->Header().Add("User-Agent", "WechatPay-C++");
			for(auto iter = params.begin(); iter != params.end(); iter++)
			{
				httpFromContent->Add(iter->first, iter->second);
			}
		}

		if (!httpRequest->SetUrl(fmt::format("{}/{}", pay_7xf::host, path)))
		{
			return false;
		}
		httpRequest->Header().SetClose();
		httpRequest->SetContent(std::move(httpFromContent));
		std::unique_ptr<http::Content> response = this->mHttp->Run(httpRequest);
		if(response == nullptr || response->GetContentType() != http::ContentType::CHUNKED)
		{
			return false;
		}
		const http::ChunkedContent * chunkedContent = response->Cast<http::ChunkedContent>();
		if(chunkedContent == nullptr)
		{
			return false;
		}
		result->append(chunkedContent->GetContent());
		return true;
	}

	bool Pay7xfComponent::Do(const std::string& path, std::map<std::string, std::string>& param, json::r::Document * result)
	{
		std::unique_ptr<std::string> response = std::make_unique<std::string>();
		if(!this->Do(path, param, response.get()))
		{
			return false;
		}
		return result->Decode(*response);
	}
}
