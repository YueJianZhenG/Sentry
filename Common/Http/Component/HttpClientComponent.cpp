//
// Created by 64658 on 2021/8/5.
//
#include"Object/App.h"
#include"Thread/TaskThread.h"
#include"HttpClientComponent.h"
#include"Method/HttpServiceMethod.h"
#include"Other/ProtoConfig.h"
#include"Other/ElapsedTimer.h"
#include"Scene/LoggerComponent.h"
#include"Scene/ThreadPoolComponent.h"
#include"Http/HttpRequestClient.h"
#include"Http/HttpHandlerClient.h"
#include"Util/FileHelper.h"
#include"Service/HttpService.h"
namespace Sentry
{

    bool HttpClientComponent::Awake()
    {
        std::string path;
        this->mCorComponent = nullptr;
        rapidjson::Document jsonDocument;
        const ServerConfig &config = App::Get().GetConfig();
        if(config.GetValue("path", "http", path))
        {
            LOG_CHECK_RET_FALSE(Helper::File::ReadJsonFile(path, jsonDocument));
            for (auto iter = jsonDocument.MemberBegin(); iter != jsonDocument.MemberEnd(); iter++)
            {
                HttpConfig *httpConfig = new HttpConfig();
                httpConfig->Url = iter->name.GetString();
                const rapidjson::Value &jsonValue = iter->value;
                httpConfig->Type = jsonValue["Type"].GetString();
                httpConfig->MethodName = jsonValue["Method"].GetString();
                httpConfig->Component = jsonValue["Component"].GetString();
                this->mHttpConfigMap.emplace(httpConfig->Url, httpConfig);
            }
        }
        return true;
    }

    bool HttpClientComponent::LateAwake()
    {
        this->mCorComponent = App::Get().GetTaskComponent();
        this->mThreadComponent = this->GetComponent<ThreadPoolComponent>();

        std::string url1 = "http://v.juhe.cn/telecode/to_telecodes.php";
        this->mCorComponent->Start([this, url1]() {

        });
        return true;
    }

    void HttpClientComponent::OnLoadData()
    {
        //std::string url = "http://langrens.oss-cn-shenzhen.aliyuncs.com/res/area/city-config.json";

        //auto response = this->Get(url);

        //LOG_WARN(response->GetContent());
        //string url = "http://yjz199595.com/logic/service/push";

//        std::string json;
//        RapidJsonWriter jsonWriter;
//        jsonWriter.Add("End", 100);
//        jsonWriter.Add("Start", 0);
//        jsonWriter.Add("RankId", 301000);
//
//        jsonWriter.WriterToStream(json);
//
//        ElapsedTimer elapsedTimer;
//        IAsioThread &thread = this->mThreadComponent->AllocateNetThread();
//        std::shared_ptr<SocketProxy> socketProxy(new SocketProxy(thread, "HttpRequest"));
//        std::shared_ptr<HttpRequestClient> httpAsyncClient(new HttpRequestClient(socketProxy));
//
//        auto response = httpAsyncClient->Post("http://127.0.0.1:80/logic/account/login", json);
//        if (response != nullptr && response->GetHttpCode() == HttpStatus::OK)
//        {
//            LOG_ERROR(response->GetContent(), " time = [", elapsedTimer.GetMs(), "ms]");
//        }

    }

    void HttpClientComponent::OnListen(std::shared_ptr<SocketProxy> socket)
    {
        std::shared_ptr<HttpHandlerClient> handlerClient(new HttpHandlerClient(socket));
        this->mCorComponent->Start(&HttpClientComponent::HandlerHttpData, this, handlerClient);
    }

    const HttpConfig *HttpClientComponent::GetHttpConfig(const std::string &url)
    {
        auto iter = this->mHttpConfigMap.find(url);
        return iter != this->mHttpConfigMap.end() ? iter->second : nullptr;
    }

    void HttpClientComponent::HandlerHttpData(std::shared_ptr<HttpHandlerClient> httpClient)
    {
        std::shared_ptr<HttpHandlerRequest> httpRequestData = httpClient->ReadHandlerContent();
        LOG_CHECK_RET(httpRequestData);
#ifdef __DEBUG__
        ElapsedTimer elapsedTimer;
        LOG_INFO("==== http request handler ====");
        LOG_INFO("url = ", httpRequestData->GetUrl());
        LOG_INFO("type = ", httpRequestData->GetMethod());
        if(!httpRequestData->GetContent().empty())
        {
            LOG_INFO("message = ", httpRequestData->GetContent());
        }
#endif
        const std::string &url = httpRequestData->GetUrl();
        const HttpConfig * httpConfig = this->GetHttpConfig(url);
        if(httpConfig == nullptr)
        {
            RapidJsonWriter jsonWriter;
            jsonWriter.Add("code", (int) XCode::CallServiceNotFound);
            jsonWriter.Add("error", fmt::format("not find url : [{0}]", url));
            httpClient->Response(HttpStatus::NOT_FOUND, jsonWriter);
            return;
        }

        if(httpConfig->Type != httpRequestData->GetMethod())
        {
            RapidJsonWriter jsonWriter;
            jsonWriter.Add("code", (int)XCode::HttpMethodNotFound);
            httpClient->Response(HttpStatus::METHOD_NOT_ALLOWED, jsonWriter);
            return;
        }
        std::shared_ptr<RapidJsonReader> jsonReader = httpRequestData->ToJsonReader();
        if (jsonReader == nullptr)
        {
            RapidJsonWriter jsonWriter;
            jsonWriter.Add("code", (int) XCode::ParseJsonFailure);
            jsonWriter.Add("error", "parse json failure");
            httpClient->Response(HttpStatus::OK, jsonWriter);
            return;
        }

        HttpService *httpService = this->GetComponent<HttpService>(httpConfig->Component);
        if (httpService == nullptr)
        {
            RapidJsonWriter jsonWriter;
            jsonWriter.Add("code", (int) XCode::CallServiceNotFound);
            jsonWriter.Add("error", "not find handler component");
            httpClient->Response(HttpStatus::OK, jsonWriter);
            return;
        }
        const std::string &method = httpConfig->MethodName;
        auto jsonResponse = httpService->Invoke(method, jsonReader);
        if (jsonResponse != nullptr)
        {
            httpClient->Response(HttpStatus::OK, *jsonResponse);
        }
#ifdef __DEBUG__
        LOG_INFO("http data response successful [", elapsedTimer.GetMs(), "ms]");
#endif
    }

    std::shared_ptr<HttpAsyncResponse> HttpClientComponent::Get(const std::string &url, int timeout)
    {
#ifdef ONLY_MAIN_THREAD
        IAsioThread &thread = App::Get().GetTaskScheduler();
#else
        IAsioThread &thread = this->mThreadComponent->AllocateNetThread();
#endif
        std::shared_ptr<SocketProxy> socketProxy(new SocketProxy(thread, "HttpRequest"));
        std::shared_ptr<HttpRequestClient> httpAsyncClient(new HttpRequestClient(socketProxy));
        return httpAsyncClient->Get(url);
    }

    std::shared_ptr<HttpAsyncResponse>
    HttpClientComponent::Post(const std::string &url, const std::string &data, int timeout)
    {
#ifdef ONLY_MAIN_THREAD
        IAsioThread &thread = App::Get().GetTaskScheduler();
#else
        IAsioThread &thread = this->mThreadComponent->AllocateNetThread();
#endif
        std::shared_ptr<SocketProxy> socketProxy(new SocketProxy(thread, "HttpRequest"));
        std::shared_ptr<HttpRequestClient> httpAsyncClient(new HttpRequestClient(socketProxy));
        return httpAsyncClient->Post(url, data);
    }

    std::shared_ptr<HttpAsyncResponse>
    HttpClientComponent::Post(const std::string &url, RapidJsonWriter &jsonWriter, int timeout)
    {
        std::string json;
        jsonWriter.WriterToStream(json);
        return this->Post(url, json);
    }

    void HttpClientComponent::Invoke(HttpRespSession *remoteRequest)
    {			
//		 HttpRequestHandler * requestHandler = remoteRequest->GetReuqestHandler();
//		 const HttpServiceConfig * httpConfig = requestHandler->GetHttpConfig();
//		 if (requestHandler != nullptr)
//		 {
//			 const std::string & method = httpConfig->Method;
//			 const std::string & service = httpConfig->Service;
//			 auto httpMethod = this->GetHttpMethod(service, method);
//			 requestHandler->SetResponseCode(httpMethod->OnResponse(remoteRequest));
//		 }
    }
}