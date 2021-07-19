﻿#include "TcpProxySession.h"
#include <Core/Applocation.h>
#include <Pool/ObjectPool.h>
#include <Manager/NetProxyManager.h>
#include <Manager/NetSessionManager.h>
#include <Coroutine/CoroutineManager.h>
namespace Sentry
{

	TcpProxySession::TcpProxySession(const std::string &address)
	{
		this->mConnectCount = 0;
		this->mAddress = address;
		this->mSessionType = SessionClient;
		Applocation *app = Applocation::Get();
		SayNoAssertRet_F(this->mCorManager = app->GetManager<CoroutineManager>());
		SayNoAssertRet_F(this->mNetManager = app->GetManager<NetSessionManager>());
		SayNoAssertRet_F(this->mNetProxyManager = app->GetManager<NetProxyManager>());
	}

	TcpProxySession::TcpProxySession(const std::string &name, const std::string &address)
	{
		this->mName = name;
		this->mConnectCount = 0;
		this->mAddress = address;
		this->mSessionType = SessionNode;
		Applocation *app = Applocation::Get();
		SayNoAssertRet_F(this->mCorManager = app->GetManager<CoroutineManager>());
		SayNoAssertRet_F(this->mNetManager = app->GetManager<NetSessionManager>());
		SayNoAssertRet_F(this->mNetProxyManager = app->GetManager<NetProxyManager>());
	}

	TcpProxySession::~TcpProxySession()
	{
		Main2NetEvent *eve = new Main2NetEvent(SocketDectoryEvent, mAddress);
		this->mNetManager->AddNetSessionEvent(eve);
#ifdef SOEASY_DEBUG
		SayNoDebugError("desctory session [" << this->mAddress << "]");
#endif
	}

	bool TcpProxySession::SendMessageData(com::NetWorkPacket *messageData)
	{
		if (messageData == nullptr)
		{
			return false;
		}
#ifdef SOEASY_DEBUG
		const std::string& method = messageData->method();
		const std::string& service = messageData->service();
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
		com::NetWorkPacket *messageData = GnetPacketPool.Create();
		if (messageData == nullptr)
		{
			return XCode::Failure;
		}
		messageData->set_method(method);
		messageData->set_service(service);
		return this->SendMessageData(messageData);
	}

	bool TcpProxySession::Notice(const std::string &service, const std::string &method, const Message &request)
	{
		if (service.empty() || method.empty())
		{
			return false;
		}
		com::NetWorkPacket *messageData = GnetPacketPool.Create();
		if (messageData == nullptr)
		{
			return XCode::Failure;
		}
		if (request.SerializeToString(&mMessageBuffer))
		{
			messageData->set_method(method);
			messageData->set_service(service);
			messageData->set_messagedata(mMessageBuffer);
			return this->SendMessageData(messageData);
		}
		GnetPacketPool.Destory(messageData);
		return false;
	}
}
