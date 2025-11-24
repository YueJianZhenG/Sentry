//
// Created by leyi on 2023/12/25.
//

#include "DingDingOutput.h"
#include "Http/Common/HttpRequest.h"
#include "Http/Common/HttpResponse.h"
#include "Util/Tools/TimeHelper.h"

namespace custom
{
	DingDingOutput::DingDingOutput(std::string  url)
			: mUrl(std::move(url))
#ifdef __ENABLE_OPEN_SSL__
			, mCtx(asio::ssl::context::sslv23)
#endif
	{

	}

	void DingDingOutput::Push(Asio::Context& io, const std::string& name, const custom::LogInfo& log)
	{
		if (log.Level < custom::LogLevel::Error)
		{
			return;
		}
		json::w::Document document;
		document.Add("msgtype", "text");
		std::unique_ptr<json::w::Value> jsonObj = document.AddObject("text");
		{
			std::string time = help::Time::GetDateString();
			switch (log.Level)
			{
			case custom::LogLevel::Debug:
				jsonObj->Add("type", "debug");
				break;
			case custom::LogLevel::Info:
				jsonObj->Add("type", "info");
				break;
			case custom::LogLevel::Warn:
				jsonObj->Add("type", "warn");
				break;
			case custom::LogLevel::Error:
				jsonObj->Add("type", "error");
				break;
			case custom::LogLevel::Fatal:
				jsonObj->Add("type", "fatal");
				break;
			}
			jsonObj->Add("file", log.File);
			jsonObj->Add("time", help::Time::GetDateString());
			jsonObj->Add("text", log.Content);
			if(log.Stack != nullptr)
			{
				jsonObj->Add("stack", log.Stack->c_str(), log.Stack->size());
			}
		}
#ifdef __ENABLE_OPEN_SSL__
		this->mClient->SetSocket(new tcp::Socket(io, this->mCtx));
#else
		this->mClient->SetSocket(new tcp::Socket(io));
#endif
		std::unique_ptr<http::Request> request = std::make_unique<http::Request>("POST");
		{
			request->SetTimeout(15);
			request->SetUrl(this->mUrl);
			request->SetContent(document);
			this->mClient->SyncSend(request);
		}
	}

	bool DingDingOutput::Start(Asio::Context& io)
	{
		try
		{
#ifdef __ENABLE_OPEN_SSL__
			this->mCtx.set_default_verify_paths();
#endif
			this->mClient = std::make_shared<http::Client>(nullptr, io);
		}
		catch (const std::system_error & error)
		{
			return false;
		}
		return true;
	}
}