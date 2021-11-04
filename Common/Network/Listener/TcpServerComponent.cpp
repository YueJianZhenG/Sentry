﻿#include "TcpServerComponent.h"

#include<Core/App.h>
#include<Util/StringHelper.h>
#include<Scene/TaskPoolComponent.h>

#include<Thread/TaskThread.h>
#include<Network/Listener/NetworkListener.h>
namespace GameKeeper
{
    bool TcpServerComponent::Awake()
    {
		ServerConfig & config = App::Get().GetConfig();
		config.GetValue("WhiteList", this->mWhiteList);
        config.GetValue("Listener","ip", this->mHostIp);
        auto taskComponent = this->GetComponent<TaskPoolComponent>();
		rapidjson::Value * jsonValue = config.GetJsonValue("Listener");
		if (jsonValue == nullptr || !jsonValue->IsObject())
		{
			return false;
		}
		for (auto iter = jsonValue->MemberBegin(); iter != jsonValue->MemberEnd(); iter++)
		{
			if (!iter->value.IsObject())
			{
				continue;
			}
            ListenConfig listenConfig;
            listenConfig.Port = 0;
            listenConfig.Ip = this->mHostIp;
            if(iter->value.HasMember("port"))
            {
                listenConfig.Port = iter->value["port"].GetUint();
                listenConfig.Count = iter->value["count"].GetInt();
            }
            listenConfig.Name = iter->name.GetString();
            listenConfig.Handler = iter->value["handler"].GetString();;

            Component * component = this->gameObject->GetComponentByName(listenConfig.Handler);

			auto socketHandler = dynamic_cast<ISocketListen*>(component);
			if(socketHandler == nullptr)
            {
                GKDebugError("not find socket handler " << listenConfig.Handler);
                return false;
            }
			NetWorkThread & netThread = taskComponent->GetNetThread();
            if(listenConfig.Port !=0)
            {
                auto listener = new NetworkListener(netThread, listenConfig);
                this->mListeners.push_back(listener);
            }
		}
		return true;
    }

    void TcpServerComponent::Start()
    {
        for (size_t index = 0; index < this->mListeners.size(); index++)
        {
            NetworkListener *listener = this->mListeners[index];
            const ListenConfig &config = listener->GetConfig();
            Component *component = this->gameObject->GetComponentByName(config.Handler);
            if (auto handler = dynamic_cast<ISocketListen *>(component))
            {
                if (listener->StartListen(handler))
                {
                    const ListenConfig &config = listener->GetConfig();
                    GKDebugLog(config.Name << " listen [" << config.Ip << ":" << config.Port << "] successful");
                }
                else
                {
                    App::Get().Stop();
                }
            }
        }
    }
}
