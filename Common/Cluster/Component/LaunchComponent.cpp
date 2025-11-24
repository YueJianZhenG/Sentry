//
// Created by zmhy0073 on 2022/10/12.
//

#include"LaunchComponent.h"
#include"Core/System/System.h"
#include "Util/Tools/String.h"
#include"Http/Component/HttpComponent.h"
#include"Cluster//Config/ClusterConfig.h"
#include"Rpc/Service/LuaRpcService.h"
#include"Http//Service/LuaHttpService.h"
#include"Lua/Component/LuaComponent.h"
#include"Rpc/Component/InnerTcpComponent.h"
#include"Rpc/Component/DispatchComponent.h"
#include"Router/Component/RouterComponent.h"

namespace acs
{
	bool LaunchComponent::Awake()
	{
		json::r::Value luaObject;
		if (this->mApp->Config().Get("lua", luaObject))
		{
			this->mApp->AddComponent<LuaComponent>();
		}

		if (ClusterConfig::Inst() == nullptr)
		{
			return true;
		}
		const NodeConfig* nodeConfig = ClusterConfig::Inst()->GetConfig();
		if (nodeConfig == nullptr)
		{
			LOG_ERROR("{} config is null", ClusterConfig::Inst()->GetConfigName());
			return false;
		}
		std::vector<std::string> components;
		std::vector<std::string> rpcService;
		std::vector<std::string> httpService;
		if(nodeConfig->GetComponents(components) > 0)
		{
			LOG_CHECK_RET_FALSE(this->AddComponent(components));
		}

		if(nodeConfig->GetRpcServices(rpcService) > 0)
		{
			LOG_CHECK_RET_FALSE(this->AddRpcService(rpcService));
		}
		if(nodeConfig->GetHttpServices(httpService) > 0)
		{
			LOG_CHECK_RET_FALSE(this->AddHttpService(httpService));
		}

		LOG_CHECK_RET_FALSE(this->LoadListenConfig());
		return true;
	}

	bool LaunchComponent::AddRpcService(const std::vector<std::string>& service)
	{
		for (const std::string& name: service)
		{
			if (!RpcConfig::Inst()->HasService(name))
			{
				LOG_ERROR("not rpc service => {}", name);
				return false;
			}
			if (this->mApp->HasComponent(name))
			{
				continue;
			}
			std::unique_ptr<Component> component = ComponentFactory::CreateComponent(name);
			if(component == nullptr)
			{
				component = std::make_unique<LuaRpcService>();
				if(component == nullptr)
				{
					return false;
				}
			}
			if(!this->mApp->AddComponent(name, std::move(component)))
			{
				LOG_ERROR("add rpc service [{}] error", name);
				return false;
			}
			//LOG_DEBUG("load rpc service => {}", name);
		}
		return true;
	}

	bool LaunchComponent::AddHttpService(const std::vector<std::string>& service)
	{
		for (const std::string& name: service)
		{
			if (this->mApp->HasComponent(name))
			{
				continue;
			}
			if (!HttpConfig::Inst()->HasService(name))
			{
				LOG_ERROR("not http service => {}", name);
				return false;
			}
			std::unique_ptr<Component> component = ComponentFactory::CreateComponent(name);
			if(component == nullptr)
			{
				component = std::make_unique<LuaHttpService>();
				if(component == nullptr)
				{
					return false;
				}
			}
			if(!this->mApp->AddComponent(name, std::move(component)))
			{
				LOG_ERROR("add http service [{}] error", name);
				return false;
			}
			//LOG_DEBUG("load http service => {}", name);
		}
		return true;
	}

	bool LaunchComponent::AddComponent(const std::vector<std::string>& components)
	{
		this->mApp->AddComponent<HttpComponent>();
		this->mApp->AddComponent<RouterComponent>();
		this->mApp->AddComponent<InnerTcpComponent>();
		this->mApp->AddComponent<DispatchComponent>();
		for (const std::string& name: components)
		{
			if (this->mApp->HasComponent(name))
			{
				continue;
			}
			if (!this->mApp->AddComponent(name))
			{
				LOG_ERROR("add {} error", name);
				return false;
			}
		}
		return true;
	}

	bool LaunchComponent::LoadListenConfig()
	{
		json::r::Value jsonObj;
		ServerConfig & config = this->mApp->GetConfig();
		{
			LOG_CHECK_RET_FALSE(config.Get("core", jsonObj));
		}
		if(!config.Get("listen", jsonObj))
		{
			return true;
		}

		std::unordered_map<std::string, int> ProtocolTypeMap = {
				{ "tcp", proto_type::tcp },
				{ "kcp", proto_type::udp },
				{ "udp", proto_type::udp },
				{ "http", proto_type::tcp },
				{ "https", proto_type::tcp },
				{ "ipc", proto_type::local }
		};
		for (const char* key: jsonObj.GetAllKey())
		{
			std::string value;
			json::r::Value jsonData;
			if (!jsonObj.Get(key, jsonData))
			{
				return false;
			}
			ListenConfig listenConfig;
			{
				listenConfig.port = 0;
				listenConfig.name = key;
			}
			if (jsonData.Get("address", listenConfig.address))
			{
				listenConfig.port = 0;
				help::Str::SplitAddr(listenConfig.address, listenConfig.proto_name, listenConfig.ip, listenConfig.port);
			}

			jsonData.Get("max_conn", listenConfig.max_conn);
			jsonData.Get("component", listenConfig.component);
#ifdef __ENABLE_OPEN_SSL__
			jsonData.Get("key", listenConfig.key);
			jsonData.Get("cert", listenConfig.cert);
#endif
			if (listenConfig.port > 0 && !listenConfig.component.empty())
			{
				auto iter = ProtocolTypeMap.find(listenConfig.proto_name);
				if (iter == ProtocolTypeMap.end())
				{
					LOG_ERROR("({}) not find proto:{}", listenConfig.name, listenConfig.proto_name)
					return false;
				}
				listenConfig.proto = iter->second;

				this->mApp->AddComponent(listenConfig.component);
				if (listenConfig.proto == proto_type::tcp)
				{

					std::string name = fmt::format("{}:ListenComponent",
							listenConfig.name);
					auto listenerComponent = std::make_unique<ListenerComponent>();
					if (!this->mApp->AddComponent(name, std::move(listenerComponent)))
					{
						return false;
					}
				}
				this->mTcpListens.emplace_back(listenConfig);
				this->mApp->AddListen(listenConfig.name, listenConfig.address);
			}
		}
		return true;
	}

	bool LaunchComponent::LateAwake()
	{
		for (const ListenConfig& listenConfig: this->mTcpListens)
		{

			INetListen* tcpListen = nullptr;
			if(listenConfig.proto == proto_type::tcp)
			{
				std::string name = fmt::format("{}:ListenComponent",
						listenConfig.name);
				tcpListen = this->GetComponent<INetListen>(name);
			}
			else
			{
				tcpListen = this->GetComponent<INetListen>(listenConfig.component);
			}
			if (tcpListen == nullptr || !tcpListen->StartListen(listenConfig))
			{
				LOG_ERROR("({}) listen [{}] fail", listenConfig.name, listenConfig.address);
				return false;
			}
			LOG_INFO("({:<6}) listen [{}] ok", listenConfig.name, listenConfig.address);
			this->mApp->AddListen(listenConfig.name, listenConfig.address);
		}
		return true;
	}

	void LaunchComponent::OnDestroy()
	{
		for (const ListenConfig& listenConfig: this->mTcpListens)
		{
			INetListen* tcpListen = nullptr;
			switch(listenConfig.proto)
			{
				case proto_type::tcp:
				{
					std::string name = fmt::format("{}:ListenComponent", listenConfig.name);
					tcpListen = this->GetComponent<INetListen>(name);
					break;
				}
				case proto_type::udp:
				{
					tcpListen = this->GetComponent<INetListen>(listenConfig.component);
					break;
				}
			}
			if(!tcpListen->StopListen())
			{
				LOG_ERROR("({}) close [{}] fail", listenConfig.name, listenConfig.address);
				continue;
			}
			LOG_INFO("({}) close [{}] ok", listenConfig.name, listenConfig.address);
		}
	}
}
