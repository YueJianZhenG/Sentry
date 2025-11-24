//
// Created by yy on 2025/11/9.
//

#include "Pay7xfService.h"
#include "Pay7xf/Component/Pay7xfComponent.h"

namespace acs
{
	Pay7xfService::Pay7xfService()
	{
		this->mPayComponent = nullptr;
	}

	bool Pay7xfService::OnInit()
	{
		BIND_COMMON_HTTP_METHOD(Pay7xfService::Create);
		BIND_COMMON_HTTP_METHOD(Pay7xfService::OnReturn);
		BIND_COMMON_HTTP_METHOD(Pay7xfService::OnNotify);
		this->mPayComponent = this->GetComponent<Pay7xfComponent>();
		return true;
	}


	int Pay7xfService::OnReturn(const http::FromContent& query, http::Response& response)
	{

		return XCode::Ok;
	}

	int Pay7xfService::Create(const http::Request& request, http::Response & response)
	{
		std::string type;
		int productId = 0;
		long long userId = 0;
		int api_version = pay_7xf::API_V2;
		const http::FromContent & fromContent = request.GetUrl().GetQuery();
		{
			fromContent.Get("version", api_version);
			LOG_ERROR_CHECK_ARGS(fromContent.Get("type", type))
			LOG_ERROR_CHECK_ARGS(fromContent.Get("user_id", userId))
		}
		pay_7xf::CreateRequest createRequest;
		if(fromContent.Get("product_id", productId))
		{
			const pay_7xf::ProductInfo * productInfo = this->mPayComponent->GetProduct(productId);
			if(productInfo == nullptr)
			{
				return XCode::ConfigError;
			}
			createRequest.name = productInfo->name;
			createRequest.money = productInfo->money;
		}
		else
		{
			LOG_ERROR_CHECK_ARGS(fromContent.Get("name", createRequest.name))
			LOG_ERROR_CHECK_ARGS(fromContent.Get("money", createRequest.money))
		}
		request.GetIp(createRequest.clientip);
		if(type != pay_7xf::type::wxpay && type != pay_7xf::type::alipay)
		{
			return XCode::Failure;
		}
		createRequest.type = type;
		this->mPayComponent->SetApiVersion(api_version);
		std::unique_ptr<pay_7xf::CreateResponse> createResponse =
			this->mPayComponent->Create(createRequest);
		if(createResponse == nullptr)
		{
			return XCode::RequestWxApiError;
		}
		response.SetCode(HttpStatus::MOVED_PERMANENTLY);
		response.Header().Add("Location", createResponse->pay_info);
		return XCode::Ok;
	}

	int Pay7xfService::OnNotify(const http::FromContent& request, http::Response& response)
	{
		std::map<std::string, std::string> paramas;
		for(auto iter = request.Begin(); iter != request.End(); iter++)
		{
			paramas.emplace(iter->first, iter->second);
		}
		std::string sign;
		pay_7xf::Notify notifyInfo;
		request.Get("sign", notifyInfo.sign);
		if(!this->mPayComponent->SignParams(paramas, sign))
		{
			return XCode::CallArgsError;
		}
		if(sign != notifyInfo.sign)
		{
			return XCode::CallArgsError;
		}
		request.Get("pid", notifyInfo.pid);
		request.Get("type", notifyInfo.type);
		request.Get("money", notifyInfo.money);
		request.Get("addtime", notifyInfo.addtime);
		request.Get("endtime", notifyInfo.endtime);
		request.Get("trade_no", notifyInfo.trade_no);
		request.Get("timestamp", notifyInfo.timestamp);
		request.Get("sign_type", notifyInfo.sign_type);
		request.Get("out_trade_no", notifyInfo.out_trade_no);
		request.Get("api_trade_no", notifyInfo.api_trade_no);
		request.Get("trade_status", notifyInfo.trade_status);
		return XCode::Ok;
	}


}