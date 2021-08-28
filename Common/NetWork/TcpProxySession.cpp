﻿#include "TcpProxySession.h"
#include <Core/App.h>
#include <Scene/SceneNetProxyComponent.h>
#include <Scene/SceneSessionComponent.h>
#include <Coroutine/CoroutineComponent.h>

namespace Sentry
{

    TcpProxySession::TcpProxySession(const std::string &address)
    {
        this->mConnectCount = 0;
        this->mAddress = address;
        this->mSessionType = SessionClient;   
		this->mCorComponent = App::Get().GetCoroutineComponent();     
        SayNoAssertRet_F(this->mNetManager = Scene::GetComponent<SceneSessionComponent>());
    }

    TcpProxySession::TcpProxySession(const std::string &name, const std::string &address)
    {
        this->mName = name;
        this->mConnectCount = 0;
        this->mAddress = address;
        this->mSessionType = SessionNode;
		this->mCorComponent = App::Get().GetCoroutineComponent();
		SayNoAssertRet_F(this->mNetManager = Scene::GetComponent<SceneSessionComponent>());
    }

    TcpProxySession::~TcpProxySession()
    {
        Main2NetEvent *eve = new Main2NetEvent(SocketDectoryEvent, mAddress);
        this->mNetManager->AddNetSessionEvent(eve);
#ifdef SOEASY_DEBUG
        SayNoDebugError("desctory session [" << this->mAddress << "]");
#endif
    }

    bool TcpProxySession::SendMessageData(NetMessageProxy *messageData)
    {
        if (messageData == nullptr)
        {
            return false;
        }
#ifdef SOEASY_DEBUG
        const std::string &method = messageData->GetMethd();
        const std::string &service = messageData->GetService();
        SayNoDebugInfo("call " << service << "." << method << " [" << this->mAddress << "]");
#endif // SOEASY_DEBUG
        Main2NetEvent *eve = new Main2NetEvent(SocketSendMsgEvent, mAddress, "", messageData);
        return this->mNetManager->AddNetSessionEvent(eve);
    }

    void TcpProxySession::StartConnect()
    {
        if (!this->IsNodeSession())
        {
            return;
        }
        this->mConnectCount++;
        Main2NetEvent *eve = new Main2NetEvent(SocketConnectEvent, mAddress);
        this->mNetManager->AddNetSessionEvent(eve);
    }

    bool TcpProxySession::Notice(const std::string &service, const std::string &method)
    {
        if (service.empty() || method.empty())
        {
            return false;
        }
		const std::string & address = this->GetAddress();
        NetMessageProxy *messageData = NetMessageProxy::Create(address, S2S_NOTICE, service, method);
        if (messageData == nullptr)
        {
            SayNoDebugError("not find method " << service << "." << method);
            return XCode::Failure;
        }
        return this->SendMessageData(messageData);
    }

    bool TcpProxySession::Notice(const std::string &service, const std::string &method, const Message &request)
    {
        if (service.empty() || method.empty())
        {
            return false;
        }
		const std::string & address = this->GetAddress();
        NetMessageProxy *messageData = NetMessageProxy::Create(address, S2S_NOTICE, service, method);
        if (messageData == nullptr)
        {
            SayNoDebugError("not find method " << service << "." << method);
            return XCode::Failure;
        }
		messageData->SetMessage(request);      
        return this->SendMessageData(messageData);
    }
}
