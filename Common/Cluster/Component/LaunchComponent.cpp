//
// Created by zmhy0073 on 2022/10/12.
//

#include"LaunchComponent.h"
#include"Entity/Actor/App.h"
#include"Util/File/FileHelper.h"
#include"Http/Component/HttpComponent.h"
#include"Cluster//Config/ClusterConfig.h"
#include"Rpc/Service/LuaRpcService.h"
#include"Http//Service/LuaHttpService.h"
#include"Lua/Component/LuaComponent.h"
#include"Rpc/Component/InnerNetComponent.h"
#include"Http/Component/HttpWebComponent.h"
#include"Rpc/Component/DispatchComponent.h"
#include "Server/Config/ServerConfig.h"
#include"Router/Component/RouterComponent.h"
namespace Tendo
{
    bool LaunchComponent::Awake()
	{
		unsigned short port = 0;
		this->mApp->AddComponent<HttpComponent>();
		this->mApp->AddComponent<RouterComponent>();
		this->mApp->AddComponent<InnerNetComponent>();
		this->mApp->AddComponent<DispatchComponent>();
		if(this->mApp->Config()->GetValue("lua") != nullptr)
		{
			this->mApp->AddComponent<LuaComponent>();
		}
		if (ServerConfig::Inst()->GetListen("http", port))
		{
			this->mApp->AddComponent<HttpWebComponent>();
		}

		std::vector<std::string> components;
		const NodeConfig* nodeConfig = ClusterConfig::Inst()->GetConfig();
		if (nodeConfig->GetComponents(components))
		{
			for (const std::string& name: components)
			{
				if (!this->mApp->HasComponent(name))
				{
					if (!this->mApp->AddComponent(name))
					{
						LOG_ERROR("add " << name << " error");
						return false;
					}
				}
			}
		}
		components.clear();
		if (nodeConfig->GetServices(components) > 0)
		{
			for (const std::string& name: components)
			{
				if(this->mApp->HasComponent(name))
				{
					continue;
				}
				const RpcServiceConfig* rpcServiceConfig = SrvRpcConfig::Inst()->GetConfig(name);
				const HttpServiceConfig* httpServiceConfig = HttpConfig::Inst()->GetConfig(name);
				if (rpcServiceConfig != nullptr)
				{
					//创建实体服务
					if (!this->mApp->AddComponent(name))
					{
						std::unique_ptr<Component> component(new LuaRpcService());
						if (!this->mApp->AddComponent(name, std::move(component)))
						{
							LOG_ERROR("add physical service [" << name << "] error");
							return false;
						}
					}
				}
				else if (httpServiceConfig != nullptr)
				{
					if (!this->mApp->AddComponent(name))
					{
						std::unique_ptr<Component> component(new LuaHttpService());
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
}
