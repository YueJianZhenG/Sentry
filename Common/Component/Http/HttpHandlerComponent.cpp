//
// Created by zmhy0073 on 2022/6/21.
//

#include"HttpHandlerComponent.h"
#include"Util/FileHelper.h"
#include"Network/Http/HttpHandlerClient.h"
#include"Component/HttpService/LocalHttpService.h"
namespace Sentry
{
    bool HttpHandlerComponent::LateAwake()
    {
        this->mTaskComponent = this->GetApp()->GetTaskComponent();
        auto iter = this->GetApp()->ComponentBegin();
        for(; iter != this->GetApp()->ComponentEnd(); iter++)
        {
            Component * component = iter->second;
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

    void HttpHandlerComponent::OnListen(std::shared_ptr<SocketProxy> socket)
    {
        const std::string &address = socket->GetAddress();

        std::shared_ptr<HttpHandlerClient> handlerClient =
                std::make_shared<HttpHandlerClient>(this, socket);

        handlerClient->StartReceive();
        this->mHttpClients.emplace(address, handlerClient);
    }

    const HttpInterfaceConfig *HttpHandlerComponent::GetConfig(const std::string &path)
    {
        auto iter = this->mHttpConfigs.find(path);
        return iter != this->mHttpConfigs.end() ? iter->second : nullptr;
    }

    void HttpHandlerComponent::ClosetHttpClient(const std::string &address)
    {
        auto iter = this->mHttpClients.find(address);
        if(iter != this->mHttpClients.end())
        {
            this->mHttpClients.erase(iter);
            LOG_DEBUG("remove http address : " << address);
        }
    }

    void HttpHandlerComponent::OnRequest(std::shared_ptr<HttpHandlerClient> httpClient)
    {
        std::shared_ptr<HttpHandlerRequest> request = httpClient->Request();
        std::shared_ptr<HttpHandlerResponse> response = httpClient->Response();

        const HttpInterfaceConfig *httpConfig = this->GetConfig(request->GetPath());
        if (httpConfig == nullptr)
        {
            response->SetCode(HttpStatus::NOT_FOUND);
            httpClient->StartWriter();
            return;
        }
        if (httpConfig->Type != request->GetMethod())
        {
            response->SetCode(HttpStatus::METHOD_NOT_ALLOWED);
            httpClient->StartWriter();
            return;
        }

        LocalHttpService *httpService = this->GetComponent<LocalHttpService>(httpConfig->Service);
        if (httpService == nullptr || !httpService->IsStartService())
        {
            response->SetCode(HttpStatus::NOT_FOUND);
            httpClient->StartWriter();
            return;
        }
        if (!httpConfig->IsAsync)
        {
            XCode code = httpService->Invoke(httpConfig->Method, request, response);
            LOG_INFO("sync call http service " << httpConfig->Service << "." << httpConfig->Method << " code = ["
                                               << (int) code << "]");
            response->AddHead("code", (int) code);
            httpClient->StartWriter();
            return;
        }
        this->mTaskComponent->Start([httpService, httpClient, httpConfig]() {

            std::shared_ptr<HttpHandlerRequest> request = httpClient->Request();
            std::shared_ptr<HttpHandlerResponse> response = httpClient->Response();

            XCode code = httpService->Invoke(httpConfig->Method, request, response);
            LOG_INFO("async call http service " << httpConfig->Service << "." << httpConfig->Method << " code = ["
                                                << (int) code << "]");
            response->AddHead("code", (int) code);
            httpClient->StartWriter();
        });
    }
}