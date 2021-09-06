﻿#include "SceneNetProxyComponent.h"

#include <Core/App.h>
#include "SceneActionComponent.h"
#include "SceneSessionComponent.h"
#include <Timer/TimerComponent.h>
#include<Service/ServiceNodeComponent.h>
#include <Service/ServiceMgrComponent.h>
#include<Service/ServiceNode.h>
namespace Sentry
{

    bool SceneNetProxyComponent::PushEventHandler(SocketEveHandler *eve)
    {
        if (eve == nullptr)
            return false;
        this->mNetEventQueue.Add(eve);
        return true;
    }

    bool SceneNetProxyComponent::DestorySession(const std::string &address)
    {
		MainSocketCloseHandler * handler = new MainSocketCloseHandler(address);
        return this->mNetWorkManager->PushEventHandler(handler);
    }

    bool SceneNetProxyComponent::SendNetMessage(PacketMapper *msgData)
    {
		const std::string &address = msgData->GetAddress();
		if (address.empty())
		{
			SayNoDebugFatal("address is null send failure");
			return false;
		}
        TcpProxySession *tcpSession = this->GetProxySession(address);
        if (tcpSession == nullptr)
        {
            delete msgData;
            return false;
        }
        return tcpSession->SendMessageData(msgData);
    }

	TcpProxySession * SceneNetProxyComponent::Create(const std::string &address, const std::string &name)
    {		
		auto iter = this->mSessionMap.find(address);
		if (iter != this->mSessionMap.end())
		{
			return iter->second;
		}
		TcpProxySession *tcpSession = new TcpProxySession(name, address);

		this->mSessionMap.emplace(address, tcpSession);
		return tcpSession;
    }

    TcpProxySession *SceneNetProxyComponent::GetProxySession(const std::string &address)
    {
        auto iter = this->mSessionMap.find(address);
        return iter != this->mSessionMap.end() ? iter->second : nullptr;
    }

    TcpProxySession *SceneNetProxyComponent::DelProxySession(const std::string &address)
    {
        auto iter = this->mSessionMap.find(address);
        if (iter != this->mSessionMap.end())
        {
            TcpProxySession *session = iter->second;
            this->mSessionMap.erase(iter);
            return session;
        }
        return nullptr;
    }

    bool SceneNetProxyComponent::Awake()
    {
		this->mReConnectTime = 3;
		this->mReConnectCount = 5;
		ServerConfig & config = App::Get().GetConfig();
		this->mTimerManager = App::Get().GetTimerComponent();
		config.GetValue("NetWork", "ReConnectTime", this->mReConnectTime);
		config.GetValue("NetWork", "ReConnectCount", this->mReConnectCount);
        SayNoAssertRetFalse_F(this->mActionManager = Scene::GetComponent<SceneActionComponent>());
        SayNoAssertRetFalse_F(this->mNetWorkManager = Scene::GetComponent<SceneSessionComponent>());     
        return true;
    }

	void SceneNetProxyComponent::ConnectAfter(const std::string & address, bool isSuc)
	{
		auto iter = this->mSessionMap.find(address);
		if (iter != this->mSessionMap.end())
		{
			TcpProxySession *session = iter->second;
			if (session != nullptr)
			{
				session->SetActive(isSuc);
				this->OnConnectSuccessful(session);
				Service::GetComponent<ServiceNodeComponent>()->GetServiceNode(address)->OnConnectNodeAfter();
			}
#ifdef _DEBUG
			SayNoDebugWarning("connect to " << address << isSuc ? " successful" : " failure");
#endif
		}
	}

	void SceneNetProxyComponent::NewConnect(const std::string & address)
	{
		TcpProxySession *session = this->GetProxySession(address);
		if (session == nullptr)
		{
			session = new TcpProxySession(address);
			this->mSessionMap.emplace(address, session);
#ifdef SOEASY_DEBUG
			SayNoDebugInfo("new session connect [" << address << "]");
#endif
		}
		this->OnNewSessionConnect(session);
	}

	void SceneNetProxyComponent::SessionError(const std::string & address)
	{
		auto iter = this->mSessionMap.find(address);
		if (iter != this->mSessionMap.end())
		{
			TcpProxySession *tcpSession = iter->second;
			if (tcpSession->IsNodeSession())
			{

				tcpSession->SetActive(false);
#ifdef SOEASY_DEBUG
				SayNoDebugError("[" << tcpSession->GetName() << " " << address << "] error");
#endif
				return;
			}
			delete tcpSession;
			this->mSessionMap.erase(iter);
#ifdef SOEASY_DEBUG
			SayNoDebugError(" remove session [" << address << "]");
#endif
		}
	}

	void SceneNetProxyComponent::ReceiveNewMessage(PacketMapper * message)
	{
		if (!this->OnRecvMessage(message))
		{
			const std::string & address = message->GetAddress();
			TcpProxySession *session = this->DelProxySession(address);
			if (session != nullptr)
			{
				delete session;
				MainSocketCloseHandler * handler = new MainSocketCloseHandler(address);
				this->mNetWorkManager->PushEventHandler(handler);
			}
		}
	}

	void SceneNetProxyComponent::OnSystemUpdate()
	{
		shared_ptr<TcpClientSession> pTcpSession = nullptr;
		SocketEveHandler *eveHandler = nullptr;
		this->mNetEventQueue.SwapQueueData();
		while (this->mNetEventQueue.PopItem(eveHandler))
		{
			eveHandler->RunHandler(this);
			delete eveHandler;
			eveHandler = nullptr;
		}
	}

    bool SceneNetProxyComponent::OnRecvMessage(PacketMapper *messageData)
    {
        if (messageData->GetMessageType() < REQUEST_END)
        {			
			ServiceMgrComponent * serviceComponent = Service::GetComponent<ServiceMgrComponent>();
			if (serviceComponent == nullptr)
			{
				return false;
			}
			return serviceComponent->HandlerMessage(messageData);    
        }
        this->mActionManager->InvokeCallback(messageData);
        return true;
    }
}