//
// Created by 64658 on 2021/8/5.
//
#include"App/App.h"
#include"Thread/TaskThread.h"
#include"HttpComponent.h"
#include"Method/HttpServiceMethod.h"
#include"Other/InterfaceConfig.h"
#include"Util/StringHelper.h"
#include"Util/DirectoryHelper.h"
#include"Component/Scene/LoggerComponent.h"
#include"Component/Scene/NetThreadComponent.h"
#include"Network//Http/HttpRequestClient.h"
#include"Network/Http/HttpHandlerClient.h"
#include"Script/Extension/Http/LuaHttp.h"
#include"Component/HttpService/LocalHttpService.h"
namespace Sentry
{

	void HttpComponent::Awake()
	{

	}

	bool HttpComponent::LateAwake()
	{
		this->mTaskComponent = this->GetApp()->GetTaskComponent();
		this->mTimeComponent = this->GetApp()->GetTimerComponent();
#ifndef ONLY_MAIN_THREAD
		this->mThreadComponent = this->GetComponent<NetThreadComponent>();
#endif
		std::vector<Component *> components;
		this->GetApp()->GetComponents(components);
		for(Component * component : components)
		{
			LocalHttpService * localHttpService = component->Cast<LocalHttpService>();
			if(localHttpService != nullptr)
			{
				std::vector<const HttpInterfaceConfig *> httpInterConfigs;
				localHttpService->GetServiceConfig().GetConfigs(httpInterConfigs);
				for(const HttpInterfaceConfig * httpInterfaceConfig : httpInterConfigs)
				{
					this->mHttpConfigs.emplace(httpInterfaceConfig->Path, httpInterfaceConfig);
				}
			}
		}

		return true;
	}

	void HttpComponent::OnListen(std::shared_ptr<SocketProxy> socket)
	{
		const std::string & address = socket->GetAddress();

		std::shared_ptr<HttpHandlerClient> handlerClient =
			std::make_shared<HttpHandlerClient>(this, socket);

		handlerClient->StartReceive();
		this->mHttpClients.emplace(address, handlerClient);
	}

    const HttpInterfaceConfig *HttpComponent::OnHandler(const HttpHandlerRequest &request, HttpHandlerResponse &response)
    {
        const std::string& path = request.GetPath();
        auto iter = this->mHttpConfigs.find(path);
        if(iter == this->mHttpConfigs.end())
        {
            response.SetCode(HttpStatus::NOT_FOUND);
            return nullptr;
        }
        const HttpInterfaceConfig* httpConfig = iter->second;
        if(httpConfig->Type != request.GetMethod())
        {
            response.SetCode(HttpStatus::METHOD_NOT_ALLOWED);
            return nullptr;
        }
        return iter->second;

    }

	void HttpComponent::OnRequest(std::shared_ptr<HttpHandlerClient> httpClient)
	{
		std::shared_ptr<HttpHandlerRequest> request = httpClient->Request();
		std::shared_ptr<HttpHandlerResponse> response = httpClient->Response();

		const HttpInterfaceConfig* httpConfig = this->OnHandler(*request, *response);
        if(httpConfig == nullptr)
        {
            httpClient->StartWriter();
            return;
        }

		LocalHttpService* httpService = this->GetComponent<LocalHttpService>(httpConfig->Service);
		if(httpService == nullptr || !httpService->IsStartService())
		{
			response->AddHead("error", "service not start");
            response->AddHead("code", (int)XCode::CallFunctionNotExist);
            httpClient->StartWriter();
			return;
		}
		if(!httpConfig->IsAsync)
		{
			XCode code = httpService->Invoke(httpConfig->Method, request, response);
            LOG_INFO("sync call http service " << httpConfig->Service << "." << httpConfig->Method << " code = [" << (int)code << "]");
            response->AddHead("code", (int)code);
			httpClient->StartWriter();
            return;
		}
		this->mTaskComponent->Start([httpService, httpClient, httpConfig, request, response]()
		{
			XCode code = httpService->Invoke(httpConfig->Method, request, response);
            LOG_INFO("async call http service " << httpConfig->Service << "." << httpConfig->Method << " code = [" << (int)code << "]");
            response->AddHead("code", (int)code);
			httpClient->StartWriter();
		});
	}

	std::shared_ptr<HttpRequestClient> HttpComponent::CreateClient()
	{
#ifdef ONLY_MAIN_THREAD
		IAsioThread& thread = this->GetApp()->GetTaskScheduler();
#else
		IAsioThread &thread = this->mThreadComponent->AllocateNetThread();
#endif
		std::shared_ptr<SocketProxy> socketProxy(new SocketProxy(thread));
		return std::make_shared<HttpRequestClient>(socketProxy, this);
	}

	std::shared_ptr<HttpAsyncResponse> HttpComponent::Get(const std::string& url, float second)
	{
        std::shared_ptr<HttpGetRequest> httpGetRequest = HttpGetRequest::Create(url);
        if(httpGetRequest == nullptr)
        {
            LOG_FATAL("parse [" << url << "] error");
            return nullptr;
        }
        std::shared_ptr<HttpTask> httpRpcTask = httpGetRequest->MakeTask(second);
        std::shared_ptr<HttpRequestClient> httpAsyncClient = this->CreateClient();

        this->AddTask(httpRpcTask);
        httpAsyncClient->Request(httpGetRequest);
        return httpRpcTask->Await();
	}

	void HttpComponent::OnLuaRegister(Lua::ClassProxyHelper& luaRegister)
	{
		luaRegister.BeginRegister<HttpComponent>();
		luaRegister.PushExtensionFunction("Get", Lua::Http::Get);
		luaRegister.PushExtensionFunction("Post", Lua::Http::Post);
		luaRegister.PushExtensionFunction("Download", Lua::Http::Download);
	}

	XCode HttpComponent::Download(const string& url, const string& path)
	{
		if(!Helper::Directory::MakeDir(path))
		{
			return XCode::Failure;
		}

		std::fstream * fs = new std::fstream();
		fs->open(path, std::ios::binary | std::ios::in | std::ios::out);
		if(!fs->is_open())
		{
			delete fs;
			return XCode::Failure;
		}
		std::shared_ptr<HttpGetRequest> httpRequest = HttpGetRequest::Create(url);
		if(httpRequest == nullptr)
		{
			return XCode::HttpUrlParseError;
		}
        std::shared_ptr<HttpTask> httpTask = httpRequest->MakeTask(0);
		std::shared_ptr<HttpRequestClient> requestClient = this->CreateClient();

        this->AddTask(httpTask);
        requestClient->Request(httpRequest, fs);
        std::shared_ptr<HttpAsyncResponse> response = httpTask->Await();

        return response->GetCode() ? XCode::Failure : XCode::Successful;
	}

	std::shared_ptr<HttpAsyncResponse> HttpComponent::Post(const std::string& url, const std::string& data, float second)
	{
        std::shared_ptr<HttpPostRequest> postRequest = HttpPostRequest::Create(url);
        if(postRequest == nullptr)
        {
            return nullptr;
        }
        postRequest->AddBody(data);
        std::shared_ptr<HttpTask> httpTask = postRequest->MakeTask(second);
		std::shared_ptr<HttpRequestClient> httpAsyncClient = this->CreateClient();

        this->AddTask(httpTask);
        httpAsyncClient->Request(postRequest);

        return httpTask->Await();
	}
	void HttpComponent::ClosetHttpClient(std::shared_ptr<HttpHandlerClient> httpClient)
	{
		const std::string & address = httpClient->GetAddress();
		auto iter = this->mHttpClients.find(address);
		if(iter != this->mHttpClients.end())
		{
			this->mHttpClients.erase(iter);
			LOG_DEBUG("remove http address : " << address);
		}
	}
}