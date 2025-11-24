//
// Created by mac on 2021/11/28.
//
#include "XCode/XCode.h"
#include"OuterTcpComponent.h"
#include"Rpc/Client/OuterTcpSession.h"
#include"Server/Config/CodeConfig.h"

#include "Entity/Actor/App.h"
#include "Core/System/System.h"


constexpr size_t FRAME_MAX_BROADCAST_COUNT = 200; //每帧最大广播数量
namespace acs
{
	OuterTcpComponent::OuterTcpComponent()
	{
		this->mWaitCount = 0;
		this->mOuter = nullptr;
		this->mMaxConnectCount = 500;
	}

	bool OuterTcpComponent::LateAwake()
	{
		json::r::Value jsonObject;
		if(ServerConfig::Inst()->Get("connect", jsonObject))
		{
			jsonObject.Get("outer", this->mMaxConnectCount);
		}
		LOG_CHECK_RET_FALSE(this->mOuter = this->mApp->GetComponent<rpc::IOuterMessage>());
		return true;
	}

	void OuterTcpComponent::OnMessage(rpc::Message * req, rpc::Message *) noexcept
	{
		std::unique_ptr<rpc::Message> message(req);
		int code = this->mOuter->OnMessage(message);
		if(code != XCode::Ok)
		{
			int sockId = message->SockId();
			this->StartClose(sockId, code);
		}
	}

	int OuterTcpComponent::Send(int id, std::unique_ptr<rpc::Message> & message) noexcept
	{
		if (message->GetType() == rpc::type::response)
		{
			--this->mWaitCount;
		}
		auto iter = this->mSessions.find(id);
		if (iter == this->mSessions.end())
		{
			return XCode::SendMessageFail;
		}
		iter->second->Send(message);
		return XCode::Ok;
	}

	bool OuterTcpComponent::OnListen(tcp::Socket * socket) noexcept
	{
		if (this->mApp->GetStatus() < ServerStatus::Start)
		{
			return false;
		}

		int sockId = this->mSocketPool.BuildNumber();
		auto iter = this->mSessions.find(sockId);
		while(iter != this->mSessions.end())
		{
			sockId = this->mSocketPool.BuildNumber();
			iter = this->mSessions.find(sockId);
		}
		Asio::Context & io = this->mApp->GetContext();
		std::shared_ptr<rpc::OuterTcpSession> outerNetClient = std::make_shared<rpc::OuterTcpSession>(sockId, this, io);
		{
			outerNetClient->StartReceive(socket);
			this->mSessions.emplace(sockId, outerNetClient);
		}
		//LOG_DEBUG("[{}] connect gate server count:{}", socket->GetAddress(), this->mGateClientMap.size())
		return true;
	}

	void OuterTcpComponent::OnClientError(int id, int code)
	{
		auto iter = this->mSessions.find(id);
		if(iter == this->mSessions.end())
		{
			return;
		}
		this->mSessions.erase(iter);
		LOG_DEBUG("remove client({}) count:{}", id, this->mSessions.size());
	}

	void OuterTcpComponent::OnSendFailure(int id, rpc::Message* req)
	{
		int sockId = 0;
		std::unique_ptr<rpc::Message> message(req);
		message->GetHead().Add(rpc::Header::code, XCode::SendMessageFail);
		if(message->GetType() == rpc::type::request && message->GetRpcId() > 0)
		{
			if(message->GetHead().Get(rpc::Header::sock_id, sockId))
			{
				this->Send(sockId, message);
			}
		}
	}

	void OuterTcpComponent::StartClose(int id, int code)
	{
		auto iter = this->mSessions.find(id);
		if(iter != this->mSessions.end())
		{
			iter->second->Stop();
		}
	}

    void OuterTcpComponent::OnRecord(json::w::Document &document)
    {
		std::unique_ptr<json::w::Value> data = document.AddObject("outer");
		{
			data->Add("wait", this->mWaitCount);
			data->Add("client", this->mSessions.size());
		}
    }

	void OuterTcpComponent::Broadcast(std::unique_ptr<rpc::Message>& message) noexcept
	{
		message->SetType(rpc::type::request);
	}

	void OuterTcpComponent::OnFrameUpdate(int elapse) noexcept
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
}