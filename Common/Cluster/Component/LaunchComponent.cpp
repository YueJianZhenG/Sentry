//
// Created by zmhy0073 on 2022/10/12.
//

#include"LaunchComponent.h"
#include"App/App.h"
#include"File/FileHelper.h"
#include"System/System.h"
#include"Config/ClusterConfig.h"
#include"Service/VirtualRpcService.h"
#include"Service/LuaPhysicalRpcService.h"
#include"Service/PhysicalRpcService.h"
#include"Service/HttpService.h"
#include"Service/LuaPhysicalHttpService.h"
#include"Component/NodeMgrComponent.h"
#include"Component/LuaScriptComponent.h"
namespace Sentry
{
    bool LaunchComponent::Awake()
    {
		if(ServerConfig::Inst()->UseLua())
		{
			this->mApp->AddComponent<LuaScriptComponent>();
		}
		unsigned short port = 0;
		if(ServerConfig::Inst()->GetListen("rpc", port))
		{
			LOG_CHECK_RET_FALSE(this->mApp->AddComponent<NodeMgrComponent>());
		}

        std::vector<std::string> components;
		const NodeConfig* nodeConfig = ClusterConfig::Inst()->GetConfig();
        if (nodeConfig->GetComponents(components))
		{
			for (const std::string& name : components)
			{
				if (!this->mApp->HasComponent(name)
					&& !this->mApp->AddComponent(name))
				{
					LOG_ERROR("add " << name << " error");
					return false;
				}
				//CONSOLE_LOG_INFO(ServerConfig::Inst()->Name() << " add component [" << name << "]");
			}
		}
        components.clear();
        if (nodeConfig->GetServices(components) > 0)
        {
            for (const std::string& name : components)
            {
                const RpcServiceConfig* rpcServiceConfig = RpcConfig::Inst()->GetConfig(name);
                const HttpServiceConfig* httpServiceConfig = HttpConfig::Inst()->GetConfig(name);
                if (rpcServiceConfig != nullptr)
                {
                    //创建实体服务
                    if (nodeConfig->IsStart(name))
                    {
                        if (!this->mApp->AddComponent(name))
                        {
                            std::unique_ptr<Component> component(new LuaPhysicalRpcService());
                            if (!this->mApp->AddComponent(name, std::move(component)))
                            {
                                LOG_ERROR("add physical service [" << name << "] error");
                                return false;
                            }
                        }
                    }
                    else //创建虚拟服务
                    {
                        std::unique_ptr<Component> component(new VirtualRpcService());
                        if (!this->mApp->AddComponent(name, std::move(component)))
                        {
                            LOG_ERROR("add virtual service [" << name << "] error");
                            return false;
                        }
                    }
                }
                else if(httpServiceConfig != nullptr)
                {
                    if (!this->mApp->AddComponent(name))
                    {
                        std::unique_ptr<Component> component(new LuaPhysicalHttpService());
                        if (!this->mApp->AddComponent(name, std::move(component)))
                        {
                            LOG_ERROR("add http service [" << name << "] error");
                            return false;
                        }
                    }
                }
                else
                {
                    LOG_ERROR("not find service config [" << name << "]");
                    return false;
                }
            }
        }
        return true;
    }

    bool LaunchComponent::Start()
    {
        const ServerConfig* config = ServerConfig::Inst();
        const ClusterConfig* clusterConfig = ClusterConfig::Inst();
        TimerComponent* timerComponent = this->GetComponent<TimerComponent>();
        NodeMgrComponent* locationComponent = this->GetComponent<NodeMgrComponent>();

        std::string location, httpLocation;
        std::vector<std::string> components;
        config->GetLocation("rpc", location);
        config->GetLocation("http", httpLocation);
        if (clusterConfig->GetConfig()->GetServices(components, true) > 0)
        {
            for (const std::string& name : components)
            {
#ifdef __DEBUG__
                long long t1 = Helper::Time::NowMilTime();
#endif
                long long id = timerComponent->DelayCall(10 * 1000, [name]()
                {
                    CONSOLE_LOG_ERROR("start service [" << name << "] timeout");
                });
                if (!this->GetComponent<IServiceBase>(name)->Start())
                {
                    LOG_ERROR("start service [" << name << "] failure");
                    return false;
                }
                timerComponent->CancelTimer(id);
                if (RpcConfig::Inst()->GetConfig(name) != nullptr)
                {
                    LOG_CHECK_RET_FALSE(!location.empty());
#ifdef __DEBUG__
                    long long t2 = Helper::Time::NowMilTime();
                    LOG_INFO("start rpc service [" << name << "] successful use time [" << t2 - t1 << "ms]");
#else
                    LOG_INFO("start rpc service [" << name << "] successful");
#endif
                }
                else if (HttpConfig::Inst()->GetConfig(name) != nullptr)
                {
                    long long t2 = Helper::Time::NowMilTime();
                    LOG_CHECK_RET_FALSE(!httpLocation.empty());
#ifdef __DEBUG__
                    LOG_INFO("start http service [" << name << "] successful use time [" << t2 - t1 << "ms]");
#else
                    LOG_INFO("start http service [" << name << "] successful");
#endif
                }
            }
        }
        if (locationComponent != nullptr)
        {
            locationComponent->AddRpcServer(ServerConfig::Inst()->Name(), location);
            locationComponent->AddHttpServer(ServerConfig::Inst()->Name(), httpLocation);
        }
        return true;
    }
	bool LaunchComponent::LateAwake()
	{
		std::vector<IServiceBase *> allServices;
		this->mApp->GetComponents(allServices);
		for(IServiceBase * service : allServices)
		{
			if(!service->Init())
			{
				return false;
			}
		}
		return true;
	}
}