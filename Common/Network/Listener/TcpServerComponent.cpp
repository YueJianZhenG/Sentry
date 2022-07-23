﻿#include"TcpServerComponent.h"
#include"App/App.h"
#include<Thread/TaskThread.h>
#include<Util/StringHelper.h>
#include<Component/Scene/NetThreadComponent.h>
#include<Network/Listener/NetworkListener.h>
namespace Sentry
{
	bool TcpServerComponent::LoadServerConfig()
	{
		const ServerConfig& config = App::Get()->GetConfig();
		const rapidjson::Value* jsonValue = config.GetJsonValue("listener");
		if (jsonValue == nullptr || !jsonValue->IsObject())
		{
			return false;
		}
		return true;
	}

	bool TcpServerComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->LoadServerConfig());
		std::vector<const ListenConfig *> listenerConfigs;
		this->GetConfig().GetListeners(listenerConfigs);
		for (const ListenConfig * listenConfig : listenerConfigs)
		{
			ISocketListen* socketHandler = this->GetComponent<ISocketListen>(listenConfig->Handler);
			if (socketHandler == nullptr)
			{
				LOG_ERROR("not find socket handler " << listenConfig->Handler);
				return false;
			}
#ifdef ONLY_MAIN_THREAD
			IAsioThread& netThread = App::Get()->GetTaskScheduler();
#else
			NetThreadComponent * threadPoolComponent = this->GetComponent<NetThreadComponent>();
			IAsioThread &netThread = threadPoolComponent->AllocateNetThread();
#endif
			if (listenConfig->Port != 0)
			{
				NetworkListener * listener =
					new NetworkListener(netThread, *listenConfig);
				this->mListeners.push_back(listener);
			}
		}
		return true;
	}

	void TcpServerComponent::OnAllServiceStart()
	{
		for (NetworkListener * listener : this->mListeners)
		{
			const ListenConfig& config = listener->GetConfig();
			ISocketListen* listenerHandler = this->GetComponent<ISocketListen>(config.Handler);
			if(listenerHandler != nullptr)
			{
                const ListenConfig& config = listener->GetConfig();
                if (!listener->StartListen(listenerHandler))
				{
                    LOG_FATAL(config.Name << " listen " << config.Address << " failure");
                    continue;
				}
                LOG_DEBUG(config.Name << " listen " << config.Address << " successful");
			}
		}
	}

	bool TcpServerComponent::StartListen(const string& name)
	{
		for (NetworkListener * listener : this->mListeners)
		{
			const ListenConfig& config = listener->GetConfig();
			ISocketListen* listenerHandler = this->GetComponent<ISocketListen>(config.Handler);
			if(listenerHandler != nullptr && config.Name == name)
			{
				if (listener->StartListen(listenerHandler))
				{
					const ListenConfig& config = listener->GetConfig();
					LOG_DEBUG(config.Name << " listen " << config.Ip << ":" << config.Port << " successful");
					return true;
				}
				return false;
			}
		}
		return false;
	}
}
