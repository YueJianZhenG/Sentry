//
// Created by yjz on 2022/1/26.
//

#include"HttpNodeService.h"
#include"Object/App.h"
#include"Service/ServiceProxy.h"
#include"Scene/ServiceProxyComponent.h"
namespace Sentry
{
    bool HttpNodeService::Awake()
    {
        BIND_HTTP_FUNCTION(HttpNodeService::Push);
        return true;
    }

    bool HttpNodeService::LateAwake()
    {
        this->mServiceComponent = this->GetComponent<ServiceProxyComponent>();
        return true;
    }

    XCode HttpNodeService::Push(const RapidJsonReader &jsonReader, RapidJsonWriter & response)
    {
        std::string address;
        std::vector<std::string> services;
        LOG_THROW_ERROR(jsonReader.TryGetValue("rpc", "address", address));
        LOG_THROW_ERROR(jsonReader.TryGetValue("rpc", "service", services));
        for(const std::string & service : services)
        {
            auto serviceProxy = this->mServiceComponent->GetServiceProxy(service);
            LOG_THROW_ERROR(serviceProxy);
            serviceProxy->AddAddress(address);
        }
        return XCode::Successful;
    }
}