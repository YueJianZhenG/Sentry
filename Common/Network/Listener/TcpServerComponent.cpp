﻿#include"TcpServerComponent.h"
#include<Core/App.h>
#include<Util/StringHelper.h>
#include<Scene/ThreadPoolComponent.h>
#include<Thread/TaskThread.h>
#include<Listener/NetworkListener.h>
namespace GameKeeper
{
    bool TcpServerComponent::Awake()
    {
		const ServerConfig & config = App::Get().GetConfig();
		config.GetValue("WhiteList", this->mWhiteList);
        config.GetValue("Listener","ip", this->mHostIp);
		rapidjson::Value * jsonValue = config.GetJsonValue("Listener");
		if (jsonValue == nullptr || !jsonValue->IsObject())
		{
			return false;
		}
        auto iter = jsonValue->MemberBegin();
		for ( ;iter != jsonValue->MemberEnd(); iter++)
		{
			if (!iter->value.IsObject())
			{
				continue;
			}
            ListenConfig * listenConfig = new ListenConfig();
            listenConfig->Port = 0;
            listenConfig->Ip = this->mHostIp;
            if(iter->value.HasMember("port"))
            {
                listenConfig->Port = iter->value["port"].GetUint();
                listenConfig->Count = iter->value["count"].GetInt();
            }
            listenConfig->Name = iter->name.GetString();
            listenConfig->Handler = iter->value["handler"].GetString();;
            this->mListenerConfigs.emplace_back(listenConfig);
		}
		return true;
    }

    void TcpServerComponent::GetListeners(std::vector<const NetworkListener *> &listeners)
    {
        listeners.clear();
        for(auto listener : this->mListeners)
        {
            listeners.emplace_back(listener);
        }
    }

    bool TcpServerComponent::LateAwake()
    {
        auto taskComponent = this->GetComponent<ThreadPoolComponent>();
        for(auto listenConfig : this->mListenerConfigs)
        {
            Component *component = this->gameObject->GetComponentByName(listenConfig->Handler);
            auto socketHandler = dynamic_cast<ISocketListen *>(component);
            if (socketHandler == nullptr)
            {
                LOG_ERROR("not find socket handler ", listenConfig->Handler);
                return false;
            }
            NetWorkThread &netThread = taskComponent->AllocateNetThread();
            if (listenConfig->Port != 0)
            {
                auto listener = new NetworkListener(netThread, *listenConfig);
                this->mListeners.push_back(listener);
            }
        }
        return true;
    }

    void TcpServerComponent::OnStart()
    {
        for (auto listener : this->mListeners)
        {
            const ListenConfig &config = listener->GetConfig();
            Component *component = this->gameObject->GetComponentByName(config.Handler);
            if (auto handler = dynamic_cast<ISocketListen *>(component))
            {
                if (listener->StartListen(handler)->Await())
                {
                    const ListenConfig &config = listener->GetConfig();
                    LOG_DEBUG(config.Name, " listen ", config.Ip, ':', config.Port, " successful");
                }
            }
        }
    }
}
