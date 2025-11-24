//
// Created by 64658 on 2021/8/5.
//
#include"HttpComponent.h"
#include"XCode/XCode.h"
#include"Server/Component/ThreadComponent.h"
#include"Http/Client/HttpClient.h"
#include"Http/Task/HttpTask.h"
#include"Entity/Actor/App.h"
#include"Lua/Engine/ModuleClass.h"
#include"Http/Common/HttpRequest.h"
#include"Util/File/DirectoryHelper.h"

#include "Lua/Lib/Lib.h"
namespace acs
{

	HttpComponent::HttpComponent()
#ifdef __ENABLE_OPEN_SSL__
			: mSslContext(asio::ssl::context::sslv23)
#endif
	{
		this->mThread = nullptr;
#ifdef __ENABLE_OPEN_SSL__
		REGISTER_JSON_CLASS_FIELD(ssl::Config, pem);
#endif
	}

	bool HttpComponent::Awake()
	{
		LuaCCModuleRegister::Add([](Lua::CCModule & ccModule) {
			ccModule.Open("net.http", lua::lib::luaopen_lhttp);
		});
#ifdef __ENABLE_OPEN_SSL__
		ssl::Config config;
		asio::error_code code;
		if(ServerConfig::Inst()->Get("ssl", config) && !config.pem.empty())
		{
			if(this->mSslContext.load_verify_file(config.pem, code).value() != Asio::OK)
			{
				LOG_ERROR("load verify file => {}", code.message());
				return false;
			}
			LOG_DEBUG("https use pem => {}", config.pem);
			return true;
		}
		if(this->mSslContext.set_default_verify_paths(code).value() != Asio::OK)
		{
			LOG_ERROR("load default verify file => {}", code.message());
			return false;
		}
		LOG_DEBUG("https use system default");
#endif
		return true;
	}

	bool HttpComponent::LateAwake()
	{
		this->mThread = this->GetComponent<ThreadComponent>();
		return true;
	}

	std::shared_ptr<http::Client> HttpComponent::CreateClient(http::Request * request)
	{
		tcp::Socket* socketProxy = nullptr;
#ifdef __ENABLE_OPEN_SSL__
		if(request->IsHttps())
		{
			asio::ssl::context * context = &this->mSslContext;
			const std::string & path = request->GetVerifyFile();
			if(!path.empty())
			{
				auto iter = this->mSslContexts.find(path);
				if(iter != this->mSslContexts.end())
				{
					context = iter->second;
				}
				else
				{
					Asio::Code code;
					context = new asio::ssl::context(asio::ssl::context::sslv23);
					if (context->load_verify_file(path, code).value() != Asio::OK)
					{
						delete context;
						LOG_ERROR("load ssh : {}", path);
						return nullptr;
					}
					this->mSslContexts.emplace(path, context);
				}
			}
			socketProxy = this->mThread->CreateSocket(*context);
		}
		else
		{
			socketProxy = this->mThread->CreateSocket();
		}
#else
		socketProxy = this->mThread->CreateSocket();
#endif
		Asio::Context & main = this->mApp->GetContext();
		return std::make_shared<http::Client>(this, socketProxy, main);
	}

	std::unique_ptr<http::Response> HttpComponent::Get(const std::string& url, int second)
	{
		std::unique_ptr<http::Request> request = std::make_unique<http::Request>("GET");
		if (!request->SetUrl(url))
		{
			LOG_ERROR("parse {} fail", url);
			return nullptr;
		}
		request->SetTimeout(second);
		return this->Do(request);
	}

	void HttpComponent::OnRecord(json::w::Document& document)
	{
		std::unique_ptr<json::w::Value> jsonObject = document.AddObject("http");
		{
			jsonObject->Add("wait", this->AwaitCount());
			jsonObject->Add("sum", this->CurrentRpcCount());
			jsonObject->Add("client", this->mUseClients.size());
		}
	}

	int HttpComponent::Send(std::unique_ptr<http::Request>& request, std::function<void(std::unique_ptr<http::Response>&)> && cb)
	{
#ifndef __ENABLE_OPEN_SSL__
		if(request->IsHttps())
		{
			const http::Url & url = request->GetUrl();
			LOG_ERROR("[not ssl] => {}", url.ToStr());
			return XCode::SendMessageFail;
		}
#endif
		int rpcId = this->BuildRpcId();
		request->Header().Add(http::Header::Connection, http::Header::Close);
		std::unique_ptr<http::Response> response = std::make_unique<http::Response>();
		std::shared_ptr<http::Client> httpAsyncClient = this->CreateClient(request.get());
		if (httpAsyncClient == nullptr)
		{
			return XCode::Failure;
		}
		this->mUseClients.emplace(rpcId, httpAsyncClient);
		this->AddTask(new HttpCallbackTask(rpcId, cb));
		httpAsyncClient->Do(request, response, rpcId);
		return XCode::Ok;
	}

	int HttpComponent::Send(std::unique_ptr<http::Request>& request, int& rpcId)
	{
#ifndef __ENABLE_OPEN_SSL__
		if(request->IsHttps())
		{
			const http::Url & url = request->GetUrl();
			LOG_ERROR("[not ssl] => {}", url.ToStr());
			return XCode::SendMessageFail;
		}
#endif
		rpcId = this->BuildRpcId();
		request->Header().Add(http::Header::Connection, http::Header::Close);
		std::unique_ptr<http::Response> response = std::make_unique<http::Response>();
		std::shared_ptr<http::Client> httpAsyncClient = this->CreateClient(request.get());
		if (httpAsyncClient == nullptr)
		{
			return XCode::Failure;
		}
		this->mUseClients.emplace(rpcId, httpAsyncClient);
		httpAsyncClient->Do(request, response, rpcId);
		return XCode::Ok;
	}


	int HttpComponent::Send(std::unique_ptr<http::Request>& request, std::unique_ptr<http::Response>& response, int& rpcId)
	{
#ifndef __ENABLE_OPEN_SSL__
		if(request->IsHttps())
		{
			const http::Url & url = request->GetUrl();
			LOG_ERROR("[not ssl] => {}", url.ToStr());
			return XCode::SendMessageFail;
		}
#endif
		rpcId = this->BuildRpcId();
		request->Header().Add(http::Header::Connection, http::Header::Close);
		std::shared_ptr<http::Client> httpAsyncClient = this->CreateClient(request.get());
		if (httpAsyncClient == nullptr)
		{
			return XCode::Failure;
		}
		this->mUseClients.emplace(rpcId, httpAsyncClient);
		httpAsyncClient->Do(request, response, rpcId);
		return XCode::Ok;
	}

	std::unique_ptr<http::Response> HttpComponent::Do(std::unique_ptr<http::Request>& request)
	{
		std::unique_ptr<http::Response> response = std::make_unique<http::Response>();
		{
			int taskId = 0;
			if(this->Send(request, response, taskId) != XCode::Ok)
			{
				return std::make_unique<http::Response>(HttpStatus::INTERNAL_SERVER_ERROR);
			}
			return this->BuildRpcTask<HttpRequestTask>(taskId)->Await();
		}
	}

	std::unique_ptr<http::Content> HttpComponent::Run(std::unique_ptr<http::Request>& request)
	{
		std::unique_ptr<http::Response> response = this->Do(request);
		if(response == nullptr || response->Code() != HttpStatus::OK)
		{
			return nullptr;
		}
		return response->MoveBody();
	}

	std::unique_ptr<http::Content> HttpComponent::Run(std::unique_ptr<http::Request>& request, std::unique_ptr<http::Content> content)
	{
		std::unique_ptr<http::Response> response = this->Do(request, std::move(content));
		if(response == nullptr || response->Code() != HttpStatus::OK)
		{
			return nullptr;
		}
		return response->MoveBody();
	}

	std::unique_ptr<http::Response> HttpComponent::Do(std::unique_ptr<http::Request>& request, std::unique_ptr<http::Content> body)
	{
		std::unique_ptr<http::Response> response = std::make_unique<http::Response>();
		{
			int taskId = 0;
			response->SetContent(body);
			if(this->Send(request, response, taskId) != XCode::Ok)
			{
				return std::make_unique<http::Response>(HttpStatus::INTERNAL_SERVER_ERROR);
			}
			return this->BuildRpcTask<HttpRequestTask>(taskId)->Await();
		}
	}

	std::unique_ptr<http::Response> HttpComponent::Post(const std::string& url, const json::w::Document& document, int second)
	{
		std::unique_ptr<http::Request> request = std::make_unique<http::Request>("POST");
		if (!request->SetUrl(url))
		{
			return nullptr;
		}
		request->SetTimeout(second);
		request->SetContent(document);
		std::unique_ptr<http::Response>response = this->Do(request);
		if (response != nullptr && response->Code() != HttpStatus::OK)
		{
			LOG_ERROR("[POST] {} fail:{}", url, HttpStatusToString(response->Code()))
		}
		return response;
	}

	std::unique_ptr<http::Response> HttpComponent::Post(const std::string& url, const std::string& data, int second)
	{
		std::unique_ptr<http::Request> request = std::make_unique<http::Request>("POST");
		if (!request->SetUrl(url))
		{
			return nullptr;
		}
		request->SetTimeout(second);
		request->SetContent(http::Header::JSON, data);
		std::unique_ptr<http::Response> response = this->Do(request);
		if (response != nullptr && response->Code() != HttpStatus::OK)
		{
			LOG_ERROR("[POST] {} fail:{}", url, HttpStatusToString(response->Code()))
		}
		return response;
	}

	void HttpComponent::OnMessage(int taskId, http::Request* req, http::Response* res) noexcept
	{
		std::unique_ptr<http::Request> request(req);
		std::unique_ptr<http::Response> response(res);
		auto iter = this->mUseClients.find(taskId);
		if (iter != this->mUseClients.end())
		{
			this->mUseClients.erase(iter);
		}
#ifdef __DEBUG__
		//if (response->Code() != HttpStatus::OK)
		{
			const std::string& error = response->GetError();
			const std::string& url = request->GetUrl().ToStr();
			const std::string& method = request->GetUrl().Method();
			LOG_BY_NAME("http", "[{}:{}ms] ({}) => {}", method, request->GetCostTime(), url, error);
		}
#endif
		this->OnResponse(taskId, response);
	}
}