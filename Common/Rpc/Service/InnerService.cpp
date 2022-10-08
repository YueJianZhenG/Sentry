//
// Created by zmhy0073 on 2022/10/8.
//

#include "InnerService.h"
#include"Component/InnerNetComponent.h"
#include"Component/InnerNetMessageComponent.h"
#ifdef __ENABLE_MYSQL__
#include"Component/MysqlDataComponent.h"
#endif

#ifdef __ENABLE_MONGODB__
#include"Component/MongoAgentComponent.h"
#endif
#include"Component/HttpComponent.h"
#include"Component/RedisSubComponent.h"
#include"Component/RedisDataComponent.h"
#include"Component/RedisRegistryComponent.h"
namespace Sentry
{
    void InnerService::Awake()
    {
#ifdef __ENABLE_MYSQL__
        this->GetApp()->AddComponent<MysqlDataComponent>();
#endif

#ifdef __ENABLE_MONGODB__
        this->GetApp()->AddComponent<MongoAgentComponent>();
#endif
        this->GetApp()->AddComponent<HttpComponent>();
        this->GetApp()->AddComponent<InnerNetComponent>();
        this->GetApp()->AddComponent<RedisSubComponent>();
        this->GetApp()->AddComponent<RedisDataComponent>();
        this->GetApp()->AddComponent<RedisRegistryComponent>();
        this->GetApp()->AddComponent<InnerNetMessageComponent>();
    }

    bool InnerService::OnStartService()
    {
        BIND_COMMON_RPC_METHOD(InnerService::Ping);
        BIND_COMMON_RPC_METHOD(InnerService::Hotfix);
        BIND_COMMON_RPC_METHOD(InnerService::StartService);
        BIND_COMMON_RPC_METHOD(InnerService::CloseService);
        RedisSubComponent * subComponent = this->GetComponent<RedisSubComponent>();
        RedisDataComponent * dataComponent = this->GetComponent<RedisDataComponent>();
        InnerNetComponent * listenComponent = this->GetComponent<InnerNetComponent>();

        LOG_CHECK_RET_FALSE(subComponent && subComponent->StartConnectRedis());
        LOG_CHECK_RET_FALSE(dataComponent && dataComponent->StartConnectRedis());
        LOG_CHECK_RET_FALSE(listenComponent && listenComponent->StartListen("rpc"));
        return true;
    }

    XCode InnerService::Ping()
    {
        return XCode::Successful;
    }

    XCode InnerService::Hotfix()
    {
        std::vector<Component *> components;
        this->GetApp()->GetComponents(components);
        for(Component * component : components)
        {
            IHotfix * hotfix = component->Cast<IHotfix>();
            if(hotfix != nullptr)
            {
                hotfix->OnHotFix();
            }
        }
        return XCode::Successful;
    }

    XCode InnerService::StartService(const com::type_string &request)
    {
        const std::string &name = request.str();
        Service *service = this->GetComponent<Service>(name);
        if (service == nullptr)
        {
            throw std::logic_error(fmt::format("not find service : {0}", name));
        }
        if (service->IsStartService())
        {
            throw std::logic_error(fmt::format("{0} already started", name));
        }
        if(!service->Start())
        {
            return XCode::Failure;
        }
        std::vector<Component *> components;
        this->GetApp()->GetComponents(components);
        for(Component * component : components)
        {
            IServiceChange * serviceChange = component->Cast<IServiceChange>();
            if(serviceChange != nullptr)
            {
                serviceChange->OnAddService(service);
            }
        }
        return XCode::Successful;
    }

    XCode InnerService::CloseService(const com::type_string &request)
    {
        const std::string &name = request.str();
        Service *service = this->GetComponent<Service>(name);
        if (service == nullptr)
        {
            throw std::logic_error(fmt::format("not find service : {0}", name));
        }
        if (!service->IsStartService())
        {
            throw std::logic_error(fmt::format("{0} not started", name));
        }
        if (!service->Close())
        {
            return XCode::Failure;
        }
        std::vector<Component *> components;
        this->GetApp()->GetComponents(components);
        for (Component *component: components)
        {
            IServiceChange *serviceChange = component->Cast<IServiceChange>();
            if (serviceChange != nullptr)
            {
                serviceChange->OnDelService(service);
            }
        }
        return XCode::Successful;
    }
}