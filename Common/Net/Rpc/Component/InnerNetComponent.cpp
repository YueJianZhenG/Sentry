﻿
#include"InnerNetComponent.h"
#include"XCode/XCode.h"
#include"Entity/Actor/App.h"
#include"Core/System/System.h"
#include"Network/Tcp/Socket.h"
#include"DispatchComponent.h"
#include"Server/Component/ThreadComponent.h"
#include"Server/Config/CodeConfig.h"
#include"Core/Event/IEvent.h"
namespace joke
{

    InnerNetComponent::InnerNetComponent()
		: ISender(rpc::Net::Tcp), mNumPool(10000)
    {
		this->mActComponent = nullptr;
		this->mDisComponent = nullptr;
        this->mNetComponent = nullptr;
    }

    bool InnerNetComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mActComponent = this->mApp->ActorMgr())
		LOG_CHECK_RET_FALSE(this->mNetComponent = this->GetComponent<ThreadComponent>())
		LOG_CHECK_RET_FALSE(this->mDisComponent = this->GetComponent<DispatchComponent>())
		return true;
	}

    void InnerNetComponent::OnMessage(rpc::Packet * message, rpc::Packet *)
	{
		int code = XCode::Ok;
		message->SetNet(rpc::Net::Tcp);
		switch (message->GetType())
		{
			case rpc::Type::Logout:
				this->StartClose(message->SockId());
				break;
			case rpc::Type::Request:
				this->OnRequest(message);
				return;
			case rpc::Type::Forward:
				this->OnForward(message);
				break;
			case rpc::Type::Response:
			{
				int socketId = 0;
				if (message->GetHead().Del("#", socketId))
				{
					this->Send(socketId, message);
					return;
				}
#ifdef __DEBUG__
				std::string func;
				int code = XCode::Ok;
				long long startTime = 0;
				message->GetHead().Get("code", code);
				const rpc::Head& head = message->ConstTempHead();
				if(head.Get("func", func) && head.Get("t", startTime))
				{
					if(code != XCode::Ok)
					{
						long long nowTme = help::Time::NowMil();
						const std::string error = CodeConfig::Inst()->GetDesc(code);
						LOG_WARN("({}ms) call [{}] code:{}", nowTme - startTime, func, error);
					}
				}
#endif
				code = this->mDisComponent->OnMessage(message);
				break;
			}
			case rpc::Type::Client:
			case rpc::Type::Broadcast:
				code = this->mDisComponent->OnMessage(message);
				break;
			default:
				code = XCode::UnKnowPacket;
			LOG_FATAL("unknown message type : {} data:{}", message->GetType(), message->ToString());
				break;
		}
		if(code != XCode::Ok)
		{
			delete message;
		}
	}

	void InnerNetComponent::OnForward(rpc::Packet* message)
	{
		int target = 0;
		int code = XCode::Ok;
		do
		{
			if(!message->GetHead().Del("tar", target))
			{
				code = XCode::CallArgsError;
				break;
			}
			message->SetType(rpc::Type::Request);
			message->GetHead().Add("#", message->SockId());
			if(this->Send(target, message) != XCode::Ok)
			{
				code = XCode::SendMessageFail;
				break;
			}
		}
		while(false);
		if(code != XCode::Ok)
		{
			message->SetType(rpc::Type::Response);
			this->Send(message->SockId(), message);
		}
	}

    void InnerNetComponent::OnSendFailure(int, rpc::Packet * message)
    {
		std::unique_ptr<rpc::Packet> data(message);
        if (message->GetType() == rpc::Type::Request)
        {
			if(message->GetRpcId() != 0)
            {
                message->SetType(rpc::Type::Response);
                message->GetHead().Add("code", XCode::NetWorkError);
                this->mDisComponent->OnMessage(data.release());
				return;
            }
        }
    }

    void InnerNetComponent::OnCloseSocket(int id, int code)
	{
		this->mClients.Del(id);
		help::InnerLogoutEvent::Trigger(id);
		LOG_ERROR("close [server:{}] code = {}", id, CodeConfig::Inst()->GetDesc(code));
	}

	bool InnerNetComponent::OnListen(tcp::Socket * socket)
	{
		int id = this->mNumPool.BuildNumber();
		while (this->mClients.Has(id))
		{
			id = this->mNumPool.BuildNumber();
		}
		rpc::InnerClient* tcpSession = new rpc::InnerClient(id, this);
		{
			if (!this->mClients.Add(id, tcpSession))
			{
				delete tcpSession;
				return false;
			}
			tcpSession->StartReceive(socket);
		}
		return true;
	}

    void InnerNetComponent::StartClose(int id)
    {
		rpc::InnerClient * tcpClient;
		if(this->mClients.Get(id, tcpClient))
		{
			tcpClient->Close();
		}
    }

    bool InnerNetComponent::GetClient(int id, rpc::InnerClient *& tcpClient)
	{
		if (this->mClients.Get(id, tcpClient))
		{
			return true;
		}
		std::string address;
		if(!this->mActComponent->GetListen(id, "rpc", address))
		{
			return false;
		}
		tcp::Socket* socketProxy = this->mNetComponent->CreateSocket(address);
		if (socketProxy == nullptr)
		{
			LOG_ERROR("parse address fail : {}", address)
			return false;
		}
		tcpClient = new rpc::InnerClient(id, this);
		{
			tcpClient->SetSocket(socketProxy);
			return this->mClients.Add(id, tcpClient);
		}
	}

    int InnerNetComponent::Send(int id, rpc::Packet * message)
    {
        if (this->mApp->Equal(id)) //发送到本机
        {
			message->SetSockId(id);
			Asio::Context & t = this->mApp->GetContext();
			t.post([this, message] { this->OnMessage(message, nullptr); });
            return XCode::Ok;
        }
#ifdef __DEBUG__
		if(message->GetType() == rpc::Type::Request)
		{
			std::string func;
			message->GetHead().Get("func", func);
			message->TempHead().Add("func", func);
			message->TempHead().Add("t", help::Time::NowMil());
		}
#endif
        rpc::InnerClient * clientSession = nullptr;
		if(!this->GetClient(id, clientSession))
		{
			LOG_ERROR("not find id : {}", id);
			return XCode::NotFoundServerRpcAddress;
		}
        return clientSession->Send(message) ? XCode::Ok : XCode::SendMessageFail;
    }

    void InnerNetComponent::OnRecord(json::w::Document &document)
    {
		std::unique_ptr<json::w::Value> data = document.AddObject("inner");
		{
			data->Add("client", (int)this->mClients.Size());
		}
    }

	void InnerNetComponent::OnRequest(rpc::Packet * message)
	{
		int code = this->mDisComponent->OnMessage(message);
		if (code != XCode::Ok)
		{
			const std::string& desc = CodeConfig::Inst()->GetDesc(code);
			LOG_ERROR("call {} code = {}", message->GetHead().GetStr("func"), desc);

			if (message->GetRpcId() == 0)
			{
				delete message;
				return;
			}
			message->Body()->clear();
			message->GetHead().Add("code", code);
			message->SetType(rpc::Type::Response);
			this->Send(message->SockId(), message);
		}
	}
}// namespace Sentry