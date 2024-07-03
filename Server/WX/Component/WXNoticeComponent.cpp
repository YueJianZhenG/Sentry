//
// Created by yy on 2024/5/18.
//

#include "WXNoticeComponent.h"
#include "Entity/Actor/App.h"
#include "Http/Common/HttpResponse.h"
#include "Util/Time/TimeHelper.h"
#include "Util/String/String.h"
#include "Http/Component/HttpComponent.h"

namespace joke
{
	WXNoticeComponent::WXNoticeComponent()
	{
		this->mHttp = nullptr;
		this->mToken.exp_time = 0;
	}

	bool WXNoticeComponent::Awake()
	{
		std::unique_ptr<json::r::Value> jsonValue;
		if (!this->mApp->Config().Get("wx", jsonValue))
		{
			return false;
		}
		LOG_CHECK_RET_FALSE(jsonValue->Get("app_id", this->mAppId))
		{
			const ServerConfig& config = this->mApp->Config();
			LOG_CHECK_RET_FALSE(config.Get("whcbqhn", jsonValue))
			LOG_CHECK_RET_FALSE(jsonValue->Get("app_id", this->mConfig.app_id));
			LOG_CHECK_RET_FALSE(jsonValue->Get("app_secret", this->mConfig.app_secret));
		}
		return true;
	}

	bool WXNoticeComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mHttp = this->GetComponent<HttpComponent>())
		return true;
	}

	void WXNoticeComponent::OnWxUnsubscribe(const std::string& openId)
	{

	}

	void WXNoticeComponent::OnWxSubscribe(const std::string& openId)
	{
		if(!this->GetAccessToken())
		{
			return;
		}
		const std::string host("https://api.weixin.qq.com/cgi-bin/user/info");
		std::string url = fmt::format("{}?access_token={}&openid={}",
				host, this->mToken.token, openId);
		http::Response * response = this->mHttp->Get(url);
		if(response == nullptr || response->GetBody() == nullptr)
		{
			return;
		}
		const http::JsonData * jsonData = response->GetBody()->To<const http::JsonData>();
		if(jsonData == nullptr)
		{
			return;
		}
		std::string unionId, publicId;
		LOG_CHECK_RET(jsonData->Get("unionid", unionId));
		LOG_CHECK_RET(jsonData->Get("openid", publicId));
		{
			
		}
	}

	bool WXNoticeComponent::GetAccessToken()
	{
		long long nowTime = help::Time::NowSec();
		if(this->mToken.token.empty() || nowTime >= this->mToken.exp_time)
		{
			const std::string token = this->mApp->Config().GetSecretKey();
			const std::string host("https://api.weixin.qq.com/cgi-bin/token");
			const std::string url = fmt::format("{}?&grant_type={}&appid={}&secret={}",
					host, "client_credential", this->mConfig.app_id, this->mConfig.app_secret);
			http::Response* response = this->mHttp->Get(url);
			if (response == nullptr || response->GetBody() == nullptr)
			{
				return false;
			}
			const http::JsonData* jsonData = response->GetBody()->To<const http::JsonData>();
			if (jsonData == nullptr)
			{
				return false;
			}
			int exp_time = 0;
			LOG_CHECK_RET_FALSE(jsonData->Get("expires_in", exp_time))
			LOG_CHECK_RET_FALSE(jsonData->Get("access_token", this->mToken.token))
			this->mToken.exp_time = help::Time::NowSec() + exp_time;
		}
		return true;
	}

	std::string WXNoticeComponent::Truncate(const std::string& thing, int count)
	{
		if(help::utf8::Length(thing) <= 20) {
			return thing;
		}
		return fmt::format("{}...", help::utf8::Sub(thing, 0, 17));
	}

	bool WXNoticeComponent::Send(const std::string& openId, const wx::NoticeData & noticeData)
	{
		if(!this->GetAccessToken())
		{
			return false;
		}
		json::w::Document message;
		message.Add("touser", openId);
		message.Add("template_id", noticeData.templateId);
		//message.Add("url", "http://weixin.qq.com/download");
		message.Add("topcolor", "#FF0000");
		if(!noticeData.path.empty())
		{
			auto jsonObject = message.AddObject("miniprogram");
			{
				jsonObject->Add("appid", this->mAppId);
				jsonObject->Add("pagepath", noticeData.path);
			}
		}

		json::w::Document document;
		for(auto iter = noticeData.data.begin(); iter != noticeData.data.end(); iter++)
		{
			const std::string & key = iter->first;
			auto jsonObject = document.AddObject(key.c_str());
			{
				jsonObject->Add("value", iter->second);
				jsonObject->Add("corlor", "#173177");
			}
		}
		message.Add("data", document);
		const std::string host("https://api.weixin.qq.com/cgi-bin/message/template/send");
		const std::string url = fmt::format("{}?access_token={}", host, this->mToken.token);
		http::Response * response = this->mHttp->Post(url, message.JsonString());
		if(response == nullptr || response->GetBody() == nullptr)
		{
			return false;
		}
		const http::JsonData* jsonData = response->GetBody()->To<const http::JsonData>();
		if (jsonData == nullptr)
		{
			return false;
		}
		return true;
	}
}