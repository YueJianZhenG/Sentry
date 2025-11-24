//
// Created by 64658 on 2025/1/2.
//

#include "XCode/XCode.h"
#include "Entity/Actor/App.h"
#include "OuterWsComponent.h"
#include "WebSocket/Common/WebSocketMessage.h"
#include "WebSocket/Client/WsClient.h"
#include "WebSocket/Client/WsSession.h"
#include "Event/Base/IEvent.h"

constexpr char msg_format = rpc::msg::text;
constexpr size_t FRAME_MAX_BROADCAST_COUNT = 200; //每帧最大广播数量

namespace acs
{
	OuterWsComponent::OuterWsComponent()
			: mNumPool(SERVER_MAX_COUNT)
	{
		this->mSumCount = 0;
		this->mOuter = nullptr;
	}

	bool OuterWsComponent::OnListen(tcp::Socket* socket) noexcept
	{
		int id = this->mNumPool.BuildNumber();
		auto iter = this->mSessions.find(id);
		while(iter != this->mSessions.end())
		{
			id = this->mNumPool.BuildNumber();
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

	bool OuterWsComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mOuter = this->mApp->GetComponent<rpc::IOuterMessage>())
		return true;
	}

	void OuterWsComponent::OnMessage(rpc::Message* req, rpc::Message* ) noexcept
	{
		++this->mSumCount;
		std::unique_ptr<rpc::Message> request(req);
		if(this->mOuter->OnMessage(request) != XCode::Ok)
		{
			int id = request->SockId();
			auto iter = this->mSessions.find(id);
			if(iter != this->mSessions.end())
			{
				iter->second->Stop();
			}
		}
	}

	void OuterWsComponent::Broadcast(std::unique_ptr<rpc::Message> & message) noexcept
	{
		message->SetType(rpc::type::request);
	}

	void OuterWsComponent::OnFrameUpdate(int elapse) noexcept
	{
		for(size_t index = 0; index < FRAME_MAX_BROADCAST_COUNT && !this->mBroadCastMessages.empty(); index++)
		{
			std::unique_ptr<rpc::Message> & broadCastMessage = this->mBroadCastMessages.front();
			{
				this->Send(broadCastMessage->SockId(), broadCastMessage);
				this->mBroadCastMessages.pop();
			}
		}
	}

	int OuterWsComponent::Send(int id, std::unique_ptr<rpc::Message> & message) noexcept
	{
		auto iter = this->mSessions.find(id);
		if(iter == this->mSessions.end())
		{
			return XCode::SendMessageFail;
		}
		iter->second->Send(message);
		return XCode::Ok;
	}

	void OuterWsComponent::OnClientError(int id, int code)
	{
		auto iter = this->mSessions.find(id);
		if(iter == this->mSessions.end())
		{
			return;
		}
		this->mSessions.erase(iter);
		LOG_WARN("remove ws client id:{} code=>{}", id, code);
	}

	void OuterWsComponent::OnRecord(json::w::Document& document)
	{
		std::unique_ptr<json::w::Value> jsonObject = document.AddObject("ws");
		{
			jsonObject->Add("sum", this->mSumCount);
			jsonObject->Add("client", this->mSessions.size());
		}
	}
}