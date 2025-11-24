//
// Created by leyi on 2024/2/22.
//
#include "Util/Crypt/Sign.h"
#ifdef __ENABLE_OPEN_SSL__

#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/err.h>

#include "WeChatComponent.h"
#include "Entity/Actor/App.h"
#include "Proto/Bson/base64.h"
#include "Util/File/FileHelper.h"
#include "Http/Common/HttpRequest.h"
#include "Http/Common/HttpResponse.h"
#include "Http/Component/HttpComponent.h"
#include "WX/Crypt/WXBizDataCrypt.h"

#include "Auth/Aes/Aes.h"
#include "Util/Tools/TimeHelper.h"

const std::string WE_CHAT_HOST = "https://api.mch.weixin.qq.com";

namespace wx
{
	inline std::string RandomStr(int length)
	{
		std::string randomString;
		randomString.resize(length);
		RAND_bytes(reinterpret_cast<unsigned char*>(&randomString[0]), length);
		const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

		for (int i = 0; i < length; ++i)
		{
			randomString[i] = base64_chars[randomString[i] % base64_chars.size()];
		}
		return randomString;
	}

	inline std::string GetOrderOverTime(long long timestamp)
	{
		struct tm pt{};
		char str[100] = { 0 };
		time_t now = (time_t)timestamp;
#ifdef __OS_WIN__
		if(localtime_s(&pt, &now) != 0)
		{
			return "";
		}
#else
		if(localtime_r(&now, &pt) != 0)
		{
			return "";
		}
#endif
		size_t size = strftime(str, sizeof(str), "%Y-%m-%dT%H:%M:%SZ", &pt);
		return { str, size };
	}

	inline bool RsaPublicEncode(const std::string& cert_file, const std::string& plaintext, std::string& output)
	{
		FILE* cert_fp = fopen(cert_file.c_str(), "r");
		if (cert_fp == nullptr)
		{
			return false;
		}

		X509* cert = PEM_read_X509(cert_fp, nullptr, nullptr, nullptr);
		fclose(cert_fp);
		if (cert == nullptr)
		{
			ERR_print_errors_fp(stderr);
			return false;
		}

		// 从证书中提取公钥
		EVP_PKEY* pubkey = X509_get_pubkey(cert);
		if (pubkey == nullptr)
		{
			X509_free(cert);
			return false;
		}

		// 使用公钥进行加密
		EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pubkey, nullptr);
		if (ctx == nullptr)
		{
			EVP_PKEY_free(pubkey);
			X509_free(cert);
			return false;
		}

		if (EVP_PKEY_encrypt_init(ctx) <= 0)
		{
			EVP_PKEY_free(pubkey);
			EVP_PKEY_CTX_free(ctx);
			X509_free(cert);
			return false;
		}

		if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
		{
			EVP_PKEY_free(pubkey);
			EVP_PKEY_CTX_free(ctx);
			X509_free(cert);
			return false;
		}

		size_t outlen;
		if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, (const unsigned char*)plaintext.c_str(), plaintext.length()) <= 0)
		{
			EVP_PKEY_free(pubkey);
			EVP_PKEY_CTX_free(ctx);
			X509_free(cert);
			return false;
		}

		std::unique_ptr<unsigned char[]> buffer = std::make_unique<unsigned char[]>(outlen);
		if (EVP_PKEY_encrypt(ctx, buffer.get(), &outlen, (const unsigned char*)plaintext.c_str(), plaintext.length()) <= 0)
		{
			ERR_print_errors_fp(stderr);
			EVP_PKEY_free(pubkey);
			EVP_PKEY_CTX_free(ctx);
			X509_free(cert);
			return false;
		}

		// 将加密结果转换为字符串
		output = _bson::base64::encode((char*)buffer.get(), outlen);

		// 释放资源
		EVP_PKEY_free(pubkey);
		EVP_PKEY_CTX_free(ctx);
		X509_free(cert);

		return true;
	}
}

namespace acs
{

	WeChatComponent::WeChatComponent()
	{
		this->mHttp = nullptr;
	}


	bool WeChatComponent::Awake()
	{
		json::r::Value jsonValue;
		if (!this->mApp->Config().Get("wx", jsonValue))
		{
			return false;
		}
		std::string keyPath, certPath;
		jsonValue.Get("app_id", this->mConfig.login.appId);
		jsonValue.Get("app_secret", this->mConfig.login.secret);

		jsonValue.Get("mch_id", this->mConfig.pay.mchId);
		jsonValue.Get("mch_num", this->mConfig.pay.mchNumber);
		jsonValue.Get("api_v3_key", this->mConfig.pay.apiV3key);
		jsonValue.Get("notify_url", this->mConfig.pay.notifyUrl);
		jsonValue.Get("complaint_url", this->mConfig.pay.complaintUrl);
		jsonValue.Get("api_key", this->mConfig.pay.apiKeyPath);
		jsonValue.Get("api_cert", this->mConfig.pay.apiCertPath);
		jsonValue.Get("public_key", this->mConfig.pay.publicKeyPath);
		help::fs::ReadTxtFile(this->mConfig.pay.apiCertPath, this->mConfig.pay.apiCert);
		return help::fs::ReadTxtFile(this->mConfig.pay.apiKeyPath, this->mConfig.pay.apiKey);
	}

	std::unique_ptr<http::Response> WeChatComponent::GetWxCode(const std::string& path)
	{
		if (!this->GetAccessToken())
		{
			return nullptr;
		}
		json::w::Document document;
		document.Add("width", 280);
		document.Add("path", path);
		document.Add("is_hyaline", true);
		std::string host("https://api.weixin.qq.com/wxa/getwxacode");
		std::string url = fmt::format("{}?access_token={}", host, this->mAccessToken.token);

		std::unique_ptr<http::Request> request = std::make_unique<http::Request>();
		{
			request->SetUrl(url);
			request->SetContent(document);
		}
		std::string contentType;
		std::unique_ptr<http::Content> res = std::make_unique<http::TextContent>();
		return this->mHttp->Do(request, std::move(res));
	}

	void WeChatComponent::OnStart()
	{
		if (!this->DownCertificates())
		{
			LOG_ERROR("down certificates fail");
		}
	}

	bool WeChatComponent::LateAwake()
	{
		this->mHttp = this->GetComponent<HttpComponent>();
		return true;
	}

	bool WeChatComponent::DownCertificates()
	{
		std::unique_ptr<http::Request> request1 = this->NewRequest("GET", "/v3/certificates");
		std::unique_ptr<http::Response> response = this->mHttp->Do(request1, std::make_unique<http::JsonContent>());
		if (response == nullptr)
		{
			return false;
		}
		json::r::Value jsonArray;
		const http::JsonContent* jsonData = response->GetBody()->To<const http::JsonContent>();
		if (!jsonData->Get("data", jsonArray))
		{
			return false;
		}

		size_t index = 0;
		json::r::Value jsonObject;
		std::string serial_no, associated_data;
		if (!jsonArray.Get(index, jsonObject))
		{
			return false;
		}
		if (!jsonObject.Get("serial_no", serial_no))
		{
			return false;
		}

		json::r::Value jsonObject2;
		if (!jsonObject.Get("encrypt_certificate", jsonObject2))
		{
			return false;
		}
		std::string ciphertext, nonce;
		jsonObject2.Get("nonce", nonce);
		jsonObject2.Get("ciphertext", ciphertext);
		jsonObject2.Get("associated_data", associated_data);

		const std::string& sessionKey = this->mConfig.pay.apiV3key;
		const std::string text = _bson::base64::decode(ciphertext);
		std::string result = aes::Aes256GcmDecode(text, sessionKey, nonce, associated_data);
		if (result.empty())
		{
			return false;
		}
		this->mConfig.pay.certNumber = serial_no;
		help::fs::WriterFile(this->mConfig.pay.publicKeyPath, result);
		LOG_INFO("down certificate file to : {}", this->mConfig.pay.publicKeyPath);
		return true;
	}

	std::unique_ptr<wx::UploadOrderResponse> WeChatComponent::GetOrderInfo(const std::string& orderId)
	{
		if (!this->GetAccessToken())
		{
			return nullptr;
		}
		json::w::Document document;
		document.Add("merchant_trade_no", orderId);
		std::string host("https://api.weixin.qq.com/wxa/sec/order/get_order");
		std::string url = fmt::format("{}?access_token={}", host, this->mAccessToken.token);
		std::unique_ptr<http::Response> httpResponse = this->mHttp->Post(url, document);
		if (httpResponse == nullptr || httpResponse->GetBody() == nullptr)
		{
			return nullptr;
		}
		const http::JsonContent* jsonData = httpResponse->GetBody()->To<const http::JsonContent>();
		if (jsonData == nullptr)
		{
			return nullptr;
		}
		std::unique_ptr<wx::UploadOrderResponse> uploadOrderResponse = std::make_unique<wx::UploadOrderResponse>();
		{
			json::r::Value jsonObject;
			jsonData->Get("errmsg", uploadOrderResponse->errmsg);
			jsonData->Get("errcode", uploadOrderResponse->errcode);
			if (jsonData->Get("order", jsonObject))
			{
				jsonObject.Get("order_state", uploadOrderResponse->order.order_state);
				jsonObject.Get("paid_amount", uploadOrderResponse->order.paid_amount);
				jsonObject.Get("description", uploadOrderResponse->order.description);
				jsonObject.Get("in_complaint", uploadOrderResponse->order.in_complaint);
				jsonObject.Get("transaction_id", uploadOrderResponse->order.transaction_id);
				jsonObject.Get("merchant_trade_no", uploadOrderResponse->order.merchant_trade_no);
			}
		}
		return uploadOrderResponse;
	}

	std::unique_ptr<wx::OrderInfoResponse> WeChatComponent::GetOrder(const std::string& orderId)
	{
		http::FromContent request;
		request.Add("mchid", this->mConfig.pay.mchId);
		std::string url = fmt::format("/v3/pay/transactions/out-trade-no/{}?mchid={}",
				orderId, this->mConfig.pay.mchId);
		std::unique_ptr<http::Request> request1 = this->NewRequest("GET", url);
		if (request1 == nullptr)
		{
			return nullptr;
		}
		std::unique_ptr<http::Response> response2 = this->mHttp->Do(request1, std::make_unique<http::JsonContent>());
		if (response2 == nullptr || response2->Code() != HttpStatus::OK)
		{
			return nullptr;
		}
		const http::JsonContent* jsonObject = response2->To<const http::JsonContent>();
		std::unique_ptr<wx::OrderInfoResponse> response1 = std::make_unique<wx::OrderInfoResponse>();
		{
			jsonObject->Get("attach", response1->attach);
			jsonObject->Get("appid", response1->appid);
			jsonObject->Get("bank_type", response1->bank_type);
			jsonObject->Get("out_trade_no", response1->out_trade_no);

			jsonObject->Get("trade_type", response1->trade_type);
			jsonObject->Get("trade_state", response1->trade_state);
			jsonObject->Get("success_time", response1->success_time);
			jsonObject->Get("trade_state_desc", response1->trade_state_desc);
			jsonObject->Get("transaction_id", response1->transaction_id);

			json::r::Value amountObject;
			if (!jsonObject->Get("amount", amountObject))
			{
				return nullptr;
			}
			amountObject.Get("total", response1->amount.total);
			amountObject.Get("currency", response1->amount.currency);
			amountObject.Get("payer_total", response1->amount.payer_total);
			amountObject.Get("payer_currency", response1->amount.payer_currency);

			json::r::Value payerObject;
			if (jsonObject->Get("payer", payerObject))
			{
				payerObject.Get("openid", response1->openid);
			}
		}
		return response1;
	}

	std::unique_ptr<http::Request> WeChatComponent::NewRequest(const char* method, const std::string& url)
	{
		std::string sign;
		std::string randStr = wx::RandomStr(32);
		long long nowTime = help::Time::NowSec();
		std::string str = fmt::format("{}\n{}\n{}\n{}\n\n", method, url, nowTime, randStr);
		if(!help::sign::WithSHA256(str, this->mConfig.pay.apiKey, sign))
		{
			return nullptr;
		}

		std::string SHA256 = "WECHATPAY2-SHA256-RSA2048";
		std::unique_ptr<http::Request> request1 = std::make_unique<http::Request>(method);
		{
			std::string auth = fmt::format(
					"{} mchid=\"{}\",nonce_str=\"{}\",timestamp=\"{}\",serial_no=\"{}\",signature=\"{}\"",
					SHA256, this->mConfig.pay.mchId, randStr, nowTime, this->mConfig.pay.mchNumber, sign);

			request1->Header().Add(http::Header::Auth, auth);
			request1->Header().Add("Accept", http::Header::JSON);
			request1->Header().Add("User-Agent", "WechatPay-C++");
			request1->SetVerifyFile(this->mConfig.pay.apiCertPath);
			request1->SetUrl(fmt::format("{}{}", WE_CHAT_HOST, url));
		}
		return request1;
	}

	std::unique_ptr<http::Request> WeChatComponent::NewRequest(const std::string& url, json::w::Document& document)
	{
		std::string body;
		std::string sign;
		document.Serialize(&body);
		std::string randStr = wx::RandomStr(32);
		long long nowTime = help::Time::NowSec();
		std::string str = fmt::format("POST\n{}\n{}\n{}\n{}\n", url, nowTime, randStr, body);
		if(!help::sign::WithSHA256(str, this->mConfig.pay.apiKey, sign))
		{
			return nullptr;
		}

		std::string SHA256 = "WECHATPAY2-SHA256-RSA2048";
		std::unique_ptr<http::Request> request1 = std::make_unique<http::Request>("POST");
		{
			std::string auth = fmt::format(
					"{} mchid=\"{}\",nonce_str=\"{}\",timestamp=\"{}\",serial_no=\"{}\",signature=\"{}\"",
					SHA256, this->mConfig.pay.mchId, randStr, nowTime, this->mConfig.pay.mchNumber, sign);

			request1->Header().Add(http::Header::Auth, auth);
			request1->Header().Add("Accept", http::Header::JSON);
			request1->Header().Add("User-Agent", "WechatPay-C++");
			request1->SetVerifyFile(this->mConfig.pay.apiCertPath);
			request1->SetUrl(fmt::format("{}{}", WE_CHAT_HOST, url));
			request1->SetContent(http::Header::JSON, body);
		}
		return request1;
	}

	void WeChatComponent::ClearAccessToken()
	{
		this->mAccessToken.exp_time = 0;
		this->mAccessToken.token.clear();
	}

	bool WeChatComponent::GetAccessToken()
	{
		long long nowTime = help::Time::NowSec();
		if (nowTime >= this->mAccessToken.exp_time || this->mAccessToken.token.empty())
		{
			std::string url = fmt::format(
					"https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid={}&secret={}",
					this->mConfig.login.appId, this->mConfig.login.secret);
			std::unique_ptr<http::Response> response = this->mHttp->Get(url);
			if (response == nullptr || response->GetBody() == nullptr)
			{
				return false;
			}
			const http::JsonContent* jsonData = response->GetBody()->To<const http::JsonContent>();
			if (jsonData == nullptr)
			{
				return false;
			}
			int expTime = 0;
			LOG_CHECK_RET_FALSE(jsonData->Get("expires_in", expTime))
			LOG_CHECK_RET_FALSE(jsonData->Get("access_token", this->mAccessToken.token))
			this->mAccessToken.exp_time = help::Time::NowSec() + expTime - 100;
		}
		return true;
	}

	std::unique_ptr<wx::UploadOrderResponse>
	WeChatComponent::UploadOrder(const wx::OrderInfo& orderInfo)
	{
		if (!this->GetAccessToken())
		{
			return nullptr;
		}

		json::w::Document document;
		//document.Add("access_token", this->mAccessToken.token);
		auto orderKey = document.AddObject("order_key");
		{
			orderKey->Add("order_number_type", 1);
			orderKey->Add("out_trade_no", orderInfo.orderId);
			orderKey->Add("mchid", this->mConfig.pay.mchId);
		}
		document.Add("logistics_type", orderInfo.logistics_type);
		document.Add("delivery_mode", 1);
		auto shipping_list = document.AddArray("shipping_list");
		{
			shipping_list->AddObject()->Add("item_desc", orderInfo.desc);
		}
		document.Add("upload_time", help::Time::GetDateDT());
		document.AddObject("payer")->Add("openid", orderInfo.openId);
		const std::string host("https://api.weixin.qq.com/wxa/sec/order/upload_shipping_info");
		const std::string url = fmt::format("{}?access_token={}", host, this->mAccessToken.token);
		std::unique_ptr<http::Response> httpResponse = this->mHttp->Post(url, document);
		if (httpResponse == nullptr || httpResponse->GetBody() == nullptr)
		{
			return nullptr;
		}
		const http::JsonContent* jsonData = httpResponse->GetBody()->To<const http::JsonContent>();
		if (jsonData == nullptr)
		{
			return nullptr;
		}
		//LOG_WARN("upload order : {}", jsonData->ToString())
		std::unique_ptr<wx::UploadOrderResponse> uploadOrderResponse
				= std::make_unique<wx::UploadOrderResponse>();
		LOG_CHECK_RET_NULL(jsonData->Get("errmsg", uploadOrderResponse->errmsg))
		LOG_CHECK_RET_NULL(jsonData->Get("errcode", uploadOrderResponse->errcode))
		return uploadOrderResponse;
	}

	bool WeChatComponent::CreateComplaintUrl(const std::string& url)
	{
//		std::string host1("/v3/merchant-service/complaints-v2");
//		std::string url1 = fmt::format("{}?begin_date=2024-06-28&end_date=2024-06-30",
//				host1);
//		std::unique_ptr<http::Request> request0 = this->NewRequest("GET", url1);
//		{
//			http::Response * response = this->mHttp->Do(std::move(request0));
//			const http::JsonData * jsonData = response->To<http::JsonData>();
//			if(jsonData != nullptr)
//			{
//				LOG_WARN("{}", jsonData->JsonStr());
//			}
//		}

		//"{\"errcode\":0,\"errmsg\":\"ok\",\"url_link\":\"https:\\/\\/wxaurl.cn\\/iC6VayGZiJo\"}"

		std::string host("/v3/merchant-service/complaint-notifications");
		std::unique_ptr<http::Request> request1 = this->NewRequest("GET", host);
		{
			std::unique_ptr<http::Response> response = this->mHttp->Do(request1);
			const http::JsonContent* jsonData = response->To<http::JsonContent>();
			if (jsonData != nullptr)
			{
				std::string mchId, address;
				jsonData->Get("mchid", mchId);
				jsonData->Get("url", address);
				if (this->mConfig.pay.mchId == mchId && address == this->mConfig.pay.complaintUrl)
				{
					return true;
				}
				LOG_WARN("mchid:{}  url:{}", mchId, address);
			}
		}
		std::unique_ptr<http::Request> request2 = this->NewRequest("DELETE", host);
		{
			this->mHttp->Do(request2);
		}
		std::string notifyUrl(url);
		if (notifyUrl.empty())
		{
			notifyUrl = this->mConfig.pay.complaintUrl;
		}
		json::w::Document message;
		message.Add("url", notifyUrl);
		//message.Add("mchid", this->mConfig.pay.mchId);
		std::unique_ptr<http::Request> request3 = this->NewRequest(host, message);
		std::unique_ptr<http::JsonContent> response2 = std::make_unique<http::JsonContent>();
		std::unique_ptr<http::Response> response = this->mHttp->Do(request3, std::move(response2));
		if (response == nullptr || response->Code() != HttpStatus::OK)
		{
			return false;
		}
		return true;
	}

	bool WeChatComponent::CreateOrder(wx::OrderInfo* orderInfo)
	{
		json::w::Document request;
		if (orderInfo->orderId.empty())
		{
			long long guid = this->mApp->MakeGuid();
			orderInfo->orderId = std::to_string(guid);
		}
		if (orderInfo->openId.empty())
		{
			LOG_ERROR("create order open_is us empty")
			return false;
		}
		request.Add("appid", this->mConfig.login.appId);
		request.Add("mchid", this->mConfig.pay.mchId);
		//request.Add("sub_mchid", this->mConfig.pay.mchId);

		request.Add("description", orderInfo->desc);
		request.Add("out_trade_no", orderInfo->orderId);
		if (!this->mConfig.pay.notifyUrl.empty())
		{
			request.Add("notify_url", this->mConfig.pay.notifyUrl);
		}
		request.Add("attach", fmt::format("{}:{}:{}",
				orderInfo->userId, orderInfo->productId, orderInfo->type));
		if (orderInfo->over_time > orderInfo->create_time)
		{
			std::string expTime = wx::GetOrderOverTime(orderInfo->over_time);
			request.Add("time_expire", expTime);
		}

		auto amountObject = request.AddObject("amount");
		{
			amountObject->Add("currency", "CNY");
			amountObject->Add("total", orderInfo->price);
		}
		const std::string url = "/v3/pay/transactions/jsapi";
		request.AddObject("payer")->Add("openid", orderInfo->openId);
		std::unique_ptr<http::Request> request1 = this->NewRequest(url, request);
		if (request1 == nullptr)
		{
			return false;
		}
		std::unique_ptr<http::JsonContent> response2 = std::make_unique<http::JsonContent>();
		std::unique_ptr<http::Response> response = this->mHttp->Do(request1, std::move(response2));
		if (response == nullptr || response->Code() != HttpStatus::OK)
		{
			return false;
		}

		const http::JsonContent* jsonData = response->To<const http::JsonContent>();
		if (jsonData == nullptr)
		{
			return false;
		}
		return jsonData->Get("prepay_id", orderInfo->PrepayId);
	}

	bool WeChatComponent::OrderSign(const std::string& prepay_id, json::w::Value* document)
	{
		std::string paySign;
		long long nowTime = help::Time::NowSec();
		const std::string nonceStr = wx::RandomStr(32);
		const std::string package = fmt::format("prepay_id={}", prepay_id);
		const std::string sign = fmt::format("{}\n{}\n{}\n{}\n",
				this->mConfig.login.appId, nowTime, nonceStr, package);
		if (!help::sign::WithSHA256(sign, this->mConfig.pay.apiKey, paySign))
		{
			return false;
		}
		document->Add("timeStamp", nowTime);
		document->Add("package", "Sign=WXPay");
		document->Add("prepayid", prepay_id);
		document->Add("nonceStr", nonceStr);
		document->Add("appid", this->mConfig.login.appId);
		document->Add("partnerid", this->mConfig.pay.mchId);
		document->Add("sign", paySign);
		return true;
	}

	bool WeChatComponent::CloseOrder(const std::string& orderId)
	{
		json::w::Document request;
		request.Add("mchid", this->mConfig.pay.mchId);
		std::string url = fmt::format("/v3/pay/transactions/out-trade-no/{}/close/", orderId);
		std::unique_ptr<http::Request> request1 = this->NewRequest(url, request);
		if (request1 == nullptr)
		{
			return false;
		}
		std::unique_ptr<http::Response> response = this->mHttp->Do(request1);
		if (response == nullptr || response->Code() != HttpStatus::OK)
		{
			return false;
		}
		LOG_INFO("{}", response->GetBody()->ToStr());
		return true;
	}

	std::unique_ptr<wx::RefundResponse>
	WeChatComponent::Refund(const wx::OrderInfo& orderInfo, const std::string& reason)
	{
		json::w::Document request;
		if (!reason.empty())
		{
			request.Add("reason", reason);
		}
		request.Add("out_trade_no", orderInfo.orderId);
		request.Add("out_refund_no", orderInfo.orderId);
		auto amountObject = request.AddObject("amount");
		{
			amountObject->Add("total", orderInfo.price);//orderInfo.price * 100);
			amountObject->Add("refund", orderInfo.refund_price);//orderInfo.price * 100);
			amountObject->Add("currency", "CNY");
		}
		std::string url = "/v3/refund/domestic/refunds";
		std::unique_ptr<http::Request> request1 = this->NewRequest(url, request);
		if (request1 == nullptr)
		{
			return nullptr;
		}
		std::unique_ptr<http::Response> response = this->mHttp->Do(request1, std::make_unique<http::JsonContent>());
		if (response == nullptr || response->Code() != HttpStatus::OK)
		{
			if(response != nullptr)
			{
				LOG_ERROR("{}", response->ToString());
			}
			return nullptr;
		}
		const http::JsonContent* jsonObject = response->To<http::JsonContent>();
		if (jsonObject == nullptr)
		{
			LOG_ERROR("{}", response->ToString());
			return nullptr;
		}
		std::unique_ptr<wx::RefundResponse> response1 = std::make_unique<wx::RefundResponse>();
		{
			jsonObject->Get("refund_id", response1->refund_id);
			jsonObject->Get("out_refund_no", response1->out_refund_no);
			jsonObject->Get("transaction_id", response1->transaction_id);
			jsonObject->Get("out_trade_no", response1->out_trade_no);

			jsonObject->Get("channel", response1->channel);
			jsonObject->Get("user_received_account", response1->user_received_account);
			jsonObject->Get("status", response1->status);

			json::r::Value amountObject;
			if (!jsonObject->Get("amount", amountObject))
			{
				LOG_ERROR("{}", jsonObject->JsonObject().ToString());
				return nullptr;
			}
			amountObject.Get("total", response1->amount.total);
			amountObject.Get("refund", response1->amount.refund);
		}
		return response1;
	}

	std::unique_ptr<wx::PhoneInfo> WeChatComponent::GetPhoneNumByCode(const std::string& code)
	{
		if (!this->GetAccessToken())
		{
			return nullptr;
		}
		const std::string host("https://api.weixin.qq.com/wxa/business/getuserphonenumber");
		std::string url = fmt::format("{}?access_token={}", host, this->mAccessToken.token);
		std::unique_ptr<http::Request> request1 = std::make_unique<http::Request>("POST");
		std::unique_ptr<http::JsonContent> responseBody = std::make_unique<http::JsonContent>();
		{
			request1->SetUrl(url);
			json::w::Document message;
			message.Add("code", code);
			request1->SetContent(message);
			std::unique_ptr<http::Response> response = this->mHttp->Do(request1, std::move(responseBody));
			if (response == nullptr || response->Code() != HttpStatus::OK)
			{
				return nullptr;
			}
			const http::JsonContent* jsonData = response->To<http::JsonContent>();
			if (jsonData == nullptr)
			{
				return nullptr;
			}
			int errcode = 0;
			std::string errmsg;
			json::r::Value jsonObject;
			jsonData->Get("errmsg", errmsg);
			jsonData->Get("errcode", errcode);
			if (!jsonData->Get("phone_info", jsonObject))
			{
				LOG_ERROR("request phone {}", errmsg);
				return nullptr;
			}
			std::unique_ptr<wx::PhoneInfo> phoneInfo = std::make_unique<wx::PhoneInfo>();
			{
				jsonObject.Get("countryCode", phoneInfo->countryCode);
				jsonObject.Get("phoneNumber", phoneInfo->phoneNumber);
				jsonObject.Get("purePhoneNumber", phoneInfo->purePhoneNumber);
			}
			return phoneInfo;
		}
	}

	std::unique_ptr<wx::UserInfo> WeChatComponent::Login(const std::string& code)
	{
		std::unique_ptr<http::Request> request1 = std::make_unique<http::Request>("GET");
		{
			http::FromContent fromData;
			fromData.Add("js_code", code);
			fromData.Add("grant_type", "authorization_code");
			fromData.Add("appid", this->mConfig.login.appId);
			fromData.Add("secret", this->mConfig.login.secret);
			request1->SetUrl("https://api.weixin.qq.com/sns/jscode2session", fromData);
		}
		std::unique_ptr<http::Response> response1 = this->mHttp->Do(request1, std::make_unique<http::JsonContent>());
		const http::JsonContent* jsonData = response1->GetBody()->To<const http::JsonContent>();
		if (jsonData == nullptr)
		{
			LOG_ERROR("login_error:{}", response1->GetBody()->ToStr());
			return nullptr;
		}
		std::unique_ptr<wx::UserInfo> loginResponse = std::make_unique<wx::UserInfo>();
		{
			jsonData->Get("unionid", loginResponse->unionid);
			LOG_CHECK_RET_NULL(jsonData->Get("openid", loginResponse->openId))
			LOG_CHECK_RET_NULL(jsonData->Get("session_key", loginResponse->sessionKey))
		}

		return loginResponse;
	}

	std::unique_ptr<wx::TransferResponse> WeChatComponent::Transfer(const wx::TransferRequest& message)
	{
		if (message.transfer_detail_list.empty())
		{
			return nullptr;
		}
		int totalAmount = 0;
		for (const wx::TransferDetail& transferDetail: message.transfer_detail_list)
		{
			totalAmount += transferDetail.transfer_amount;
		}
		json::w::Document document;
		document.Add("total_num", (int)message.transfer_detail_list.size());
		document.Add("appid", this->mConfig.login.appId);
		document.Add("out_batch_no", message.out_batch_no);
		document.Add("batch_name", message.batch_name);
		document.Add("batch_remark", message.batch_remark);
		document.Add("total_amount", totalAmount);
		document.Add("transfer_scene_id", message.transfer_scene_id);
		auto detailArray = document.AddArray("transfer_detail_list");
		for (const wx::TransferDetail& transferDetail: message.transfer_detail_list)
		{
			std::unique_ptr<json::w::Value> detailObject = detailArray->AddObject();
			{
				detailObject->Add("out_detail_no", transferDetail.out_detail_no);
				detailObject->Add("transfer_amount", transferDetail.transfer_amount);
				detailObject->Add("transfer_remark", transferDetail.transfer_remark);
				detailObject->Add("openid", transferDetail.openid);
				if (transferDetail.transfer_amount >= 200000) //最大两千人命币
				{
					LOG_ERROR("transfer amount:{}", transferDetail.user_name);
					return nullptr;
				}
			}
		}
		const std::string url = "/v3/transfer/batches";
		std::unique_ptr<http::Request> request = this->NewRequest(url, document);
		{
			request->SetVerifyFile(this->mConfig.pay.publicKeyPath);
			request->Header().Add("Wechatpay-Serial", this->mConfig.pay.certNumber);
		}
		std::unique_ptr<http::Response> response = this->mHttp->Do(request, std::make_unique<http::JsonContent>());
		if (response == nullptr || response->Code() != HttpStatus::OK)
		{
			return nullptr;
		}
		const http::JsonContent* document1 = response->To<http::JsonContent>();
		if (document1 == nullptr)
		{
			LOG_ERROR("transfer_error:{}", response->GetBody()->ToStr());
			return nullptr;
		}
		std::unique_ptr<wx::TransferResponse> transferResponse = std::make_unique<wx::TransferResponse>();
		{
			document1->Get("batch_id", transferResponse->batch_id);
			document1->Get("create_time", transferResponse->create_time);
			document1->Get("batch_status", transferResponse->batch_status);
			document1->Get("out_batch_no", transferResponse->out_batch_no);
		}
		return transferResponse;
	}

	std::unique_ptr<wx::LinkInfo> WeChatComponent::GetLinkUrl(const std::string& path, const std::string& query)
	{
		if (!this->GetAccessToken())
		{
			return nullptr;
		}
		json::w::Document document;
		document.Add("path", path);
		document.Add("query", query);
		std::string host1("https://api.weixin.qq.com/wxa/generate_urllink");
		std::string url1 = fmt::format("{}?access_token={}", host1, this->mAccessToken.token);
		std::unique_ptr<http::Request> request0 = std::make_unique<http::Request>("POST");
		{
			request0->SetUrl(url1);
			request0->SetContent(document);
			std::unique_ptr<http::Response> response = this->mHttp->Do(request0);
			if (response == nullptr || response->Code() != HttpStatus::OK)
			{
				return nullptr;
			}
			const http::JsonContent* jsonData = response->To<http::JsonContent>();
			if (jsonData == nullptr)
			{
				return nullptr;
			}
			std::unique_ptr<wx::LinkInfo> linkInfo = std::make_unique<wx::LinkInfo>();
			{
				jsonData->Get("errmsg", linkInfo->errmsg);
				jsonData->Get("url_link", linkInfo->url_link);
			}
			return linkInfo;
		}
	}

	std::unique_ptr<wx::LinkInfo> WeChatComponent::GetSchemeUrl(const std::string& path, const std::string& query)
	{
		if (!this->GetAccessToken())
		{
			return nullptr;
		}
		json::w::Document document;
		document.Add("path", path);
		document.Add("query", query);
		std::string host1("https://api.weixin.qq.com/wxa/generatescheme");
		std::string url1 = fmt::format("{}?access_token={}", host1, this->mAccessToken.token);
		std::unique_ptr<http::Request> request0 = std::make_unique<http::Request>("POST");
		{
			request0->SetUrl(url1);
			request0->SetContent(document);
			std::unique_ptr<http::Response> response = this->mHttp->Do(request0);
			if (response == nullptr || response->Code() != HttpStatus::OK)
			{
				return nullptr;
			}
			const http::JsonContent* jsonData = response->To<http::JsonContent>();
			if (jsonData == nullptr)
			{
				return nullptr;
			}
			std::unique_ptr<wx::LinkInfo> linkInfo = std::make_unique<wx::LinkInfo>();
			{
				jsonData->Get("errmsg", linkInfo->errmsg);
				jsonData->Get("openlink", linkInfo->url_link);
			}
			return linkInfo;
		}
	}

	std::unique_ptr<wx::ComplainInfo> WeChatComponent::DecodeComplain(const wx::ComplainRequest& message)
	{
		const wx::ComplainResource& resource = message.resource;
		const std::string& sessionKey = this->mConfig.pay.apiV3key;
		const std::string text = _bson::base64::decode(resource.ciphertext);
		std::string result = aes::Aes256GcmDecode(text, sessionKey, resource.nonce, resource.associated_data);
		if (result.empty())
		{
			return nullptr;
		}
		json::r::Document document;
		if (!document.Decode(result, YYJSON_READ_INSITU))
		{
			return nullptr;
		}
		std::unique_ptr<wx::ComplainInfo> complainInfo = std::make_unique<wx::ComplainInfo>();
		{
			document.Get("action_type", complainInfo->action_type);
			document.Get("complaint_id", complainInfo->complaint_id);
		}
		return complainInfo;
	}

	std::unique_ptr<wx::PayResponse> WeChatComponent::DecodeResponse(const wx::PayMessage& message) const
	{
		const std::string& sessionKey = this->mConfig.pay.apiV3key;
		const std::string text = _bson::base64::decode(message.ciphertext);
		std::string result = aes::Aes256GcmDecode(text, sessionKey, message.nonce, message.associated_data);
		if (result.empty())
		{
			return nullptr;
		}
		json::r::Document document;
		if (!document.Decode(result, YYJSON_READ_INSITU))
		{
			return nullptr;
		}
		std::unique_ptr<wx::PayResponse> payResponse = std::make_unique<wx::PayResponse>();
		{
			document.Get("appid", payResponse->appid);
			document.Get("mchid", payResponse->mchid);
			document.Get("out_trade_no", payResponse->out_trade_no);
			document.Get("transaction_id", payResponse->transaction_id);
			document.Get("trade_state", payResponse->trade_state);
			document.Get("bank_type", payResponse->bank_type);
			document.Get("attach", payResponse->attach);
			json::r::Value jsonObject;
			if (document.Get("payer", jsonObject))
			{
				jsonObject.Get("openid", payResponse->open_id);
			}
			if (document.Get("amount", jsonObject))
			{
				jsonObject.Get("total", payResponse->amount.total);
				jsonObject.Get("payer_total", payResponse->amount.payer_total);
				jsonObject.Get("currency", payResponse->amount.currency);
				jsonObject.Get("payer_currency", payResponse->amount.payer_currency);
			}
		}
		return payResponse;
	}

	std::unique_ptr<json::r::Document> WeChatComponent::DecodeData(const std::string& iv,
			const std::string& sessionKey, const std::string& encryptedData)
	{
		std::string data;
		WxBizDataSecure::WXBizDataCrypt oWXBizDataCrypt(sessionKey);
		if (oWXBizDataCrypt.DecryptData(encryptedData, iv, data) != 0)
		{
			return nullptr;
		}
		LOG_WARN("decode info = {}", data);
		std::unique_ptr<json::r::Document> document = std::make_unique<json::r::Document>();
		if (!document->Decode(data))
		{
			return nullptr;
		}
		return document;
	}

	bool WeChatComponent::AddTemplate(const std::string& id, const std::string& desc)
	{
		if (!this->GetAccessToken())
		{
			return false;
		}
		json::w::Document document;
		document.Add("tid", id);
		document.AddArray("kidList");
		document.Add("sceneDesc", desc);
		const std::string host("https://api.weixin.qq.com/wxaapi/newtmpl/addtemplate");
		const std::string url = fmt::format("{}?access_token={}", host, this->mAccessToken.token);
		std::unique_ptr<http::Response> httpResponse = this->mHttp->Post(url, document);

		if (httpResponse == nullptr || httpResponse->GetBody() == nullptr)
		{
			return false;
		}
		const http::JsonContent* jsonData = httpResponse->GetBody()->To<const http::JsonContent>();
		if (jsonData == nullptr)
		{
			return false;
		}
		int errorCode = 0;
		std::string priTmplId;
		LOG_CHECK_RET_FALSE(jsonData->Get("errcode", errorCode))
		LOG_CHECK_RET_FALSE(jsonData->Get("priTmplId", priTmplId))
		return true;
	}

	bool WeChatComponent::SetJumpPath(const std::string& path)
	{
		if (!this->GetAccessToken())
		{
			return false;
		}
		json::w::Document request;
		request.Add("path", path);
		//POST https://api.weixin.qq.com/wxa/sec/order/set_msg_jump_path?access_token=ACCESS_TOKEN
		const std::string host("https://api.weixin.qq.com/wxa/sec/order/set_msg_jump_path");
		const std::string url = fmt::format("{}?access_token={}", host, this->mAccessToken.token);
		std::unique_ptr<http::Response> response = this->mHttp->Post(url, request);
		if (response == nullptr || response->GetBody() == nullptr)
		{
			return false;
		}
		const http::JsonContent* jsonData = response->GetBody()->To<const http::JsonContent>();
		if (jsonData == nullptr)
		{
			return false;
		}
		int errorCode = 0;
		if (!jsonData->Get("errcode", errorCode) && errorCode == 0)
		{
			std::string errorMsg;
			if (jsonData->Get("errmsg", errorMsg))
			{
				LOG_ERROR("set jump path error:{}", errorMsg);
			}
			return false;
		}
		return true;
	}

	const http::JsonContent* WeChatComponent::Get(const std::string& url, http::FromContent& fromData)
	{
		if (!this->GetAccessToken())
		{
			return nullptr;
		}
		fromData.Add("access_token", this->mAccessToken.token);
		std::unique_ptr<http::Request> request = std::make_unique<http::Request>("GET");
		{
			request->SetUrl(url, fromData);
			std::unique_ptr<http::Response> response = this->mHttp->Do(request);
			if (response == nullptr || response->Code() != HttpStatus::OK)
			{
				return nullptr;
			}
			return response->To<http::JsonContent>();
		}
	}
}
#endif