//
// Created by zmhy0073 on 2022/6/21.
//

#include"HttpWebComponent.h"
#include"Util/FileHelper.h"
#include"Network/Http/HttpHandlerClient.h"
#include"Component/Scene/NetThreadComponent.h"
#include"Component/HttpService/LocalHttpService.h"
namespace Sentry
{
    bool HttpWebComponent::LateAwake()
    {
        HttpListenComponent::LateAwake();
        auto iter = this->GetApp()->ComponentBegin();
        for (; iter != this->GetApp()->ComponentEnd(); iter++)
        {
            Component *component = iter->second;
            LocalHttpService *localHttpService = component->Cast<LocalHttpService>();
            if (localHttpService != nullptr)
            {
                std::vector<const HttpInterfaceConfig *> httpInterConfigs;
                localHttpService->GetServiceConfig().GetConfigs(httpInterConfigs);
                for (const HttpInterfaceConfig *httpInterfaceConfig: httpInterConfigs)
                {
                    this->mHttpConfigs.emplace(httpInterfaceConfig->Path, httpInterfaceConfig);
                }
            }
        }
        this->mTaskComponent = this->GetApp()->GetTaskComponent();
        return true;
    }


    const HttpInterfaceConfig *HttpWebComponent::GetConfig(const std::string &path)
    {
        auto iter = this->mHttpConfigs.find(path);
        if(iter != this->mHttpConfigs.end())
        {
            return iter->second;
        }
        iter = this->mHttpConfigs.begin();
        for(; iter != this->mHttpConfigs.end(); iter++)
        {
            size_t pos = path.find(iter->first);
            if(pos != std::string::npos)
            {
                return iter->second;
            }
        }
        return nullptr;
    }

    void HttpWebComponent::OnRequest(std::shared_ptr<HttpHandlerClient> httpClient)
    {
        assert(this->GetApp()->IsMainThread());
        std::shared_ptr<HttpHandlerRequest> request = httpClient->Request();

        const HttpData & httpData = request->GetData();
        const HttpInterfaceConfig *httpConfig = this->GetConfig(httpData.mPath);
        if (httpConfig == nullptr)
        {
            httpClient->StartWriter(HttpStatus::NOT_FOUND);
            CONSOLE_LOG_ERROR("[" << request->GetUrl() << "] " << HttpStatusToString(HttpStatus::NOT_FOUND));
            return;
        }

        if (!httpConfig->Type.empty() && httpConfig->Type != httpData.mMethod)
        {
            httpClient->StartWriter(HttpStatus::METHOD_NOT_ALLOWED);
            CONSOLE_LOG_ERROR("[" << request->GetUrl() << "] " << HttpStatusToString(HttpStatus::METHOD_NOT_ALLOWED));
            return;
        }

        LocalHttpService *httpService = this->GetComponent<LocalHttpService>(httpConfig->Service);
        if (httpService == nullptr || !httpService->IsStartService())
        {
            httpClient->StartWriter(HttpStatus::NOT_FOUND);
            CONSOLE_LOG_ERROR("[" << httpData.mPath << "] " << HttpStatusToString(HttpStatus::NOT_FOUND));
            return;
        }
        if (!httpConfig->IsAsync)
        {
            std::shared_ptr<HttpHandlerResponse> response = httpClient->Response();
            XCode code = httpService->Invoke(httpConfig->Method, request, response);
            {
                response->AddHead("code", (int) code);
                httpClient->StartWriter(HttpStatus::OK);
            }
        }
        else
        {
            this->mTaskComponent->Start([httpService, httpClient, httpConfig]() {

                std::shared_ptr<HttpHandlerRequest> request = httpClient->Request();
                std::shared_ptr<HttpHandlerResponse> response = httpClient->Response();
                XCode code = httpService->Invoke(httpConfig->Method, request, response);
                {
                    response->AddHead("code", (int) code);
                    httpClient->StartWriter(HttpStatus::OK);
                }
            });
        }
    }
}