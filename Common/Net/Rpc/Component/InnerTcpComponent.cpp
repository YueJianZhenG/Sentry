
#include"InnerTcpComponent.h"
#include"XCode/XCode.h"
#include"Entity/Actor/App.h"
#include"Core/System/System.h"
#include"Network/Tcp/Socket.h"
#include"DispatchComponent.h"
#include"Server/Component/ThreadComponent.h"
#include"Event/Base/IEvent.h"
namespace acs
{

    InnerTcpComponent::InnerTcpComponent()
		: mNumPool(SERVER_MAX_COUNT)
    {
		this->mWaitCount = 0;
		this->mActor = nullptr;
		this->mThread = nullptr;
		this->mDispatch = nullptr;
    }

    bool InnerTcpComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mActor = this->GetComponent<NodeComponent>())
		LOG_CHECK_RET_FALSE(this->mThread = this->GetComponent<ThreadComponent>())
		LOG_CHECK_RET_FALSE(this->mDispatch = this->GetComponent<DispatchComponent>())
		return true;
	}

    void InnerTcpComponent::OnMessage(rpc::Message * request, rpc::Message *) noexcept
	{
    	std::unique_ptr<rpc::Message> message(request);
		message->SetNet(rpc::net::tcp);
		switch (message->GetType())
		{
			case rpc::type::logout:
				this->StartClose(message->SockId());
				break;
			case rpc::type::request:
				if(message->GetRpcId() > 0)
				{
					this->mWaitCount++;
				}
				this->mDispatch->OnMessage(message);
				break;
			case rpc::type::forward:
				this->OnForward(message);
				break;
			case rpc::type::response:
			{

//#ifdef __DEBUG__
//				std::string func;
//				int code = XCode::Ok;
//				long long startTime = 0;
//				message->GetHead().Get("code", code);
//				const rpc::Head& head = message->ConstTempHead();
//				if (head.Get(rpc::Header::func, func) && head.Get("t", startTime))
//				{
//					if (code != XCode::Ok)
//					{
//						long long nowTme = help::Time::NowMil();
//						const std::string error = CodeConfig::Inst()->GetDesc(code);
//						LOG_WARN("({}ms) call [{}] code:{}", nowTme - startTime, func, error);
//					}
//				}
//#endif
				if(message->GetRpcId() > 0)
				{
					this->mWaitCount++;
				}
				this->mDispatch->OnMessage(message);
				break;
			}
			case rpc::type::client:
			case rpc::type::broadcast:
				this->mDispatch->OnMessage(message);
				break;
			default:
			LOG_FATAL("unknown message type : {} data:{}", message->GetType(), message->ToString());
				break;
		}
	}

	int InnerTcpComponent::OnForward(std::unique_ptr<rpc::Message> & message) noexcept
	{
		int target = 0;
		int code = XCode::Ok;
		do
		{
			if(!message->GetHead().Del(rpc::Header::forward_tar, target))
			{
				code = XCode::CallArgsError;
				break;
			}
			message->SetType(rpc::type::request);
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
			message->SetType(rpc::type::response);
			return this->Send(message->SockId(), message);
		}
		return XCode::Ok;
	}

    void InnerTcpComponent::OnSendFailure(int, rpc::Message * req)
    {
    	std::unique_ptr<rpc::Message> message(req);
		//LOG_DEBUG("send fail => {}", message->ToString())
        if (message->GetType() == rpc::type::request && message->GetRpcId() > 0)
		{
			message->SetType(rpc::type::response);
			message->GetHead().Add(rpc::Header::code, XCode::NetWorkError);
			this->mDispatch->OnMessage(message);
		}
    }

	bool InnerTcpComponent::OnListen(tcp::Socket * socket) noexcept
	{
		int id = this->mNumPool.BuildNumber();
		auto iter = this->mSessions.find(id);
		while(iter != this->mSessions.end())
		{
			id = this->mNumPool.BuildNumber();
			iter = this->mSessions.find(id);
		}
		Asio::Context & io = this->mApp->GetContext();
		std::shared_ptr<rpc::InnerTcpClient> tcpSession = std::make_shared<rpc::InnerTcpClient>(id, this, false, io);
		{

			tcpSession->StartReceive(socket);
			this->mSessions.emplace(id, tcpSession);
		}
		return true;
	}

	void InnerTcpComponent::OnClientError(int id, int code)
	{
		auto iter = this->mClients.find(id);
		if(iter != this->mClients.end())
		{
			this->mClients.erase(iter);
			help::InnerClientErrorEvent::Trigger(id);
		}
		else
		{
			iter = this->mSessions.find(id);
			if(iter != this->mSessions.end())
			{
				this->mSessions.erase(iter);
				help::InnerClientErrorEvent::Trigger(id);
			}
		}
	}

    void InnerTcpComponent::StartClose(int id)
    {
		auto iter = this->mClients.find(id);
		if(iter != this->mClients.end())
		{
			iter->second->Close();
			this->mClients.erase(iter);
			help::InnerClientErrorEvent::Trigger(id);
		}
		else
		{
			iter = this->mSessions.find(id);
			if(iter != this->mSessions.end())
			{
				iter->second->Close();
				this->mSessions.erase(iter);
				help::InnerClientErrorEvent::Trigger(id);
			}
		}
    }

	rpc::InnerTcpClient * InnerTcpComponent::GetClient(int id)
	{
		auto iter = this->mClients.find(id);
		if(iter != this->mClients.end())
		{
			return iter->second.get();
		}
		iter = this->mSessions.find(id);
		if(iter != this->mSessions.end())
		{
			return iter->second.get();
		}
		std::string address;
		if(!this->mActor->GetListen(id, "rpc", address))
		{
			return nullptr;
		}
		tcp::Socket* socketProxy = this->mThread->CreateSocket(address);
		if (socketProxy == nullptr)
		{
			LOG_ERROR("parse address fail : {}", address)
			return nullptr;
		}
		Asio::Context & io = this->mApp->GetContext();
		std::shared_ptr<rpc::InnerTcpClient> tcpClient = std::make_shared<rpc::InnerTcpClient>(id, this, true, io);
		{
			tcpClient->SetSocket(socketProxy);
			this->mClients.emplace(id, tcpClient);
		}
		return tcpClient.get();
	}

    int InnerTcpComponent::Send(int id, std::unique_ptr<rpc::Message> & message) noexcept
    {
        rpc::InnerTcpClient * clientSession = this->GetClient(id);
		if(clientSession == nullptr)
		{
			LOG_ERROR("not find actor:{}", id);
			return XCode::NotFoundServerRpcAddress;
		}
        return clientSession->Send(message) ? XCode::Ok : XCode::SendMessageFail;
    }

    void InnerTcpComponent::OnRecord(json::w::Document &document)
    {
		std::unique_ptr<json::w::Value> data = document.AddObject("inner");
		{
			data->Add("wait", this->mWaitCount);
			data->Add("client", this->mClients.size());
			data->Add("session", this->mSessions.size());
		}
    }

	void InnerTcpComponent::Broadcast(std::unique_ptr<rpc::Message>& message) noexcept
	{
		auto iter = this->mSessions.begin();
		for(; iter != this->mSessions.end(); iter++)
		{
			if(iter->first >= SERVER_MAX_COUNT)
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
}// namespace Sentry