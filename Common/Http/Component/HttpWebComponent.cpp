//
// Created by zmhy0073 on 2022/6/21.
//

#include"HttpWebComponent.h"
#include"File/FileHelper.h"
#include"Config/ServiceConfig.h"
#include"Client/HttpHandlerClient.h"
#include"Service/LocalHttpService.h"
#include"Component/NetThreadComponent.h"
namespace Sentry
{
    bool HttpWebComponent::LateAwake()
    {
        std::vector<LocalHttpService *> httpServices;
        if (this->mApp->GetComponents(httpServices) <= 0)
        {
            return false;
        }
        this->mTaskComponent = this->mApp->GetTaskComponent();
        return true;
    }

    void HttpWebComponent::OnRequest(std::shared_ptr<HttpHandlerClient> httpClient)
    {
        assert(this->mApp->IsMainThread());
        std::shared_ptr<HttpHandlerRequest> request = httpClient->Request();

        const HttpData & httpData = request->GetData();
        const HttpMethodConfig *httpConfig = ServiceConfig::Inst()->GetHttpMethodConfig(httpData.mPath);
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