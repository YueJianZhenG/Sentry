//
// Created by 64658 on 2025/1/2.
//

#include "XCode/XCode.h"
#include "Entity/Actor/App.h"
#include "InnerWsComponent.h"
#include "WebSocket/Common/WebSocketMessage.h"
#include "WebSocket/Client/WsClient.h"
#include "WebSocket/Client/WsSession.h"
#include "Server/Component/ThreadComponent.h"
#include "Rpc/Component/DispatchComponent.h"

constexpr char msg_format = rpc::msg::text;

namespace acs
{
	InnerWsComponent::InnerWsComponent()
		: mClientPool(SERVER_MAX_COUNT)
	{
		this->mActor = nullptr;
		this->mThread = nullptr;
		this->mDispatch = nullptr;
	}

	bool InnerWsComponent::OnListen(tcp::Socket* socket) noexcept
	{
		int id = this->mClientPool.BuildNumber();
		auto iter = this->mSessions.find(id);
		while(iter != this->mSessions.end())
		{
			id = this->mClientPool.BuildNumber();
			iter = this->mSessions.find(id);
		}
		Asio::Context & io = this->mApp->GetContext();
		std::shared_ptr<ws::Session> sessionClient = std::make_unique<ws::Session>(id, this, io, msg_format);
		{
			sessionClient->StartReceive(socket);
			this->mSessions.emplace(id, sessionClient);
		}
		return true;
	}

	bool InnerWsComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mActor = this->GetComponent<NodeComponent>())
		LOG_CHECK_RET_FALSE(this->mThread = this->GetComponent<ThreadComponent>())
		LOG_CHECK_RET_FALSE(this->mDispatch = this->GetComponent<DispatchComponent>())
		return true;
	}

	void InnerWsComponent::OnMessage(rpc::Message* req, rpc::Message* response) noexcept
	{
		req->SetSource(rpc::source::server);
		std::unique_ptr<rpc::Message> message(req);
		if (this->mDispatch->OnMessage(message) != XCode::Ok)
		{
			//LOG_DEBUG("[{}] message = {}", message->SockId(), message->ToString())
		}
	}

	int InnerWsComponent::Send(int id, std::unique_ptr<rpc::Message> & message) noexcept
	{
		auto iter = this->mSessions.find(id);
		if(iter != this->mSessions.end())
		{
			iter->second->Send(message);
			return XCode::Ok;
		}
		auto iter1 = this->mClients.find(id);
		if(iter1 != this->mClients.end())
		{
			iter1->second->Send(message);
			return XCode::Ok;
		}
		std::string address;
		if(!this->mActor->GetListen(id, "ws", address))
		{
			return XCode::NotFoundActorAddress;
		}
		Asio::Context & context = this->mApp->GetContext();
		tcp::Socket * tcpSocket = this->mThread->CreateSocket(address);
		std::shared_ptr<ws::Client> requestClient = std::make_shared<ws::Client>(id, this, context, msg_format);
		{
			requestClient->SetSocket(tcpSocket);
			this->mClients.emplace(id, requestClient);
		}
		requestClient->Send(message);
		return XCode::Ok;
	}

	int InnerWsComponent::Send(int id, std::unique_ptr<ws::Message>& message) noexcept
	{
		auto iter = this->mSessions.find(id);
		if(iter != this->mSessions.end())
		{
			iter->second->Send(message);
			return XCode::Ok;
		}
		auto iter1 = this->mClients.find(id);
		if(iter1 == this->mClients.end())
		{
			return XCode::SendMessageFail;
		}
		iter1->second->Send(message);
		return XCode::Ok;
	}


	int InnerWsComponent::Create(const std::string& address)
	{
		int id = this->mClientPool.BuildNumber();
		Asio::Context & context = this->mApp->GetContext();
		tcp::Socket * tcpSocket = this->mThread->CreateSocket(address);
		std::shared_ptr<ws::Client> requestClient = std::make_shared<ws::Client>(id, this, context, msg_format);
		{
			requestClient->SetSocket(tcpSocket);
			this->mClients.emplace(id, requestClient);
		}
		return id;
	}


	void InnerWsComponent::OnClientError(int id, int code)
	{
		auto iter = this->mSessions.find(id);
		if(iter != this->mSessions.end())
		{
			this->mSessions.erase(iter);
			LOG_WARN("remove ws client id:{} code=>{}", id, code);
		}
	}

	void InnerWsComponent::Broadcast(std::unique_ptr<rpc::Message>& message) noexcept
	{
		auto iter = this->mSessions.begin();
		for (; iter != this->mSessions.end(); iter++)
		{
			std::unique_ptr<rpc::Message> request = message->Clone();
			{
				request->SetSockId(iter->first);
				request->SetType(rpc::type::broadcast);
				iter->second->Send(request);
			}
		}
	}
}