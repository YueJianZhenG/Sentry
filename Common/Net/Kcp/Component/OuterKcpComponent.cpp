//
// Created by 64658 on 2024/10/24.
//

#include "XCode/XCode.h"
#include "Util/Tools/String.h"
#include "Message/c2s/c2s.pb.h"
#include "OuterKcpComponent.h"
#include "Rpc/Common/Message.h"
#include "Entity/Actor/App.h"
#include "Util/Tools/TimeHelper.h"
#include "Kcp/Common/KcpClient.h"
#include "Server/Config/CodeConfig.h"
#include "Rpc/Component/DispatchComponent.h"
#include "Server/Component/ThreadComponent.h"
#include "Core/Thread/ThreadSync.h"

namespace acs
{
	OuterKcpComponent::OuterKcpComponent()
		: mRecvBuf(mRecvBuffer, kcp::BUFFER_COUNT), mNumberFactory(10)
	{
		this->mDispatch = nullptr;
		this->mMsg = rpc::msg::text;
	}

	bool OuterKcpComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mDispatch = this->GetComponent<DispatchComponent>())
		return true;
	}

	bool OuterKcpComponent::StopListen()
	{
		asio::error_code code;
		this->mKcpServer->close(code);
		return true;
	}

	void OuterKcpComponent::StartReceive()
	{
		auto callback = [this](const asio::error_code& code, size_t size)
		{
			if (code.value() == Asio::OK)
			{
				this->OnReceiveMessage(size);
			}
			else if (code == asio::error::operation_aborted)
			{
				return;
			}
			this->StartReceive();
		};
		this->mKcpServer->async_receive_from(this->mRecvBuf, this->mRemotePoint, callback);
	}

	void OuterKcpComponent::GetAddress(std::string& address)
	{
		unsigned short port = this->mRemotePoint.port();
		std::string ip = this->mRemotePoint.address().to_string();
		address = fmt::format("{}:{}", ip, port);
	}

	bool OuterKcpComponent::OnReceiveMessage(size_t size)
	{
		this->mDecodeBuffer.resize(kcp::BUFFER_COUNT);
		kcp::Session * kcpSession = this->GetSession();
		char* buffer = const_cast<char*>(this->mDecodeBuffer.data());
		int len = kcpSession->Decode(mRecvBuffer, (long)size, buffer);
		if (len <= 0)
		{
			return false;
		}
		int sockId = kcpSession->GetSockId();
		std::stringstream strBuffer(this->mDecodeBuffer);
		std::unique_ptr<rpc::Message> rpcPacket = std::make_unique<rpc::Message>();
		{
			rpcPacket->SetSockId(sockId);
			rpcPacket->SetMsg(this->mMsg);
			rpcPacket->SetNet(rpc::net::kcp);
			rpcPacket->SetSource(rpc::source::client);
			if (rpcPacket->OnRecvMessage(strBuffer, len) != 0)
			{
				CONSOLE_LOG_ERROR("{}", std::string(this->mRecvBuffer, len))
				return false;
			}
			//LOG_DEBUG("[{}] {}", sockId, rpcPacket->GetHead().GetStr(rpc::Header::func));
		}
		this->OnMessage(sockId, rpcPacket.release(), nullptr);
		return true;
	}

	bool OuterKcpComponent::StartListen(const acs::ListenConfig& listen)
	{
		try
		{
			unsigned int port = listen.port;
			Asio::Context& io = this->mApp->GetContext();
			asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), port);
			this->mKcpServer = std::make_unique<asio_udp::socket>(io, endpoint);
		}
		catch (const asio::error_code & e)
		{
			LOG_ERROR("[kcp listen] => {}", e.message());
			return false;
		}
		this->StartReceive();
		return true;
	}

	void OuterKcpComponent::OnSystemUpdate(long long now) noexcept
	{
		auto iter = this->mSessions.begin();
		for(;iter != this->mSessions.end(); iter++)
		{
			iter->second->Update(now);
		}
	}

	int OuterKcpComponent::Send(int id, std::unique_ptr<rpc::Message> & message) noexcept
	{
		if(message->GetType() == rpc::type::close)
		{
			this->Remove(id);
			return XCode::Ok;
		}
		message->SetMsg(this->mMsg);
		auto iter = this->mSessions.find(id);
		if(iter == this->mSessions.end())
		{
			return XCode::SendMessageFail;
		}
		iter->second->Send(message);
		return XCode::Ok;
	}

	void OuterKcpComponent::OnMessage(int id, rpc::Message *req, rpc::Message *response) noexcept
	{
		std::unique_ptr<rpc::Message> request(req);
		switch(request->GetType())
		{
			case rpc::type::logout:
			{
				this->Remove(id);
				break;
			}
			case rpc::type::request:
				this->OnRequest(request);
				break;
			default:
				this->Remove(id);
				break;
		}
	}

	void OuterKcpComponent::Remove(int id)
	{
		auto iter = this->mSessions.find(id);
		if(iter == this->mSessions.end())
		{
			return;
		}
		const std::string & address = iter->second->GetAddress();
		{
			LOG_DEBUG("remove kcp client => {}", address)
			auto iter1 = this->mClients.find(address);
			if(iter1 != this->mClients.end())
			{
				this->mClients.erase(iter1);
			}
		}
		this->mSessions.erase(iter);
	}

	int OuterKcpComponent::OnRequest(std::unique_ptr<rpc::Message> & message)
	{
		int code = this->mDispatch->OnMessage(message);
		if (code != XCode::Ok)
		{
			const rpc::Head & head = message->ConstHead();
			const std::string& desc = CodeConfig::Inst()->GetDesc(code);
			LOG_ERROR("call {} code = {}", head.GetStr(rpc::Header::func), desc);

			if (message->GetRpcId() == 0)
			{
				return XCode::Failure;
			}
			message->Body()->clear();
			message->SetType(rpc::type::response);
			message->GetHead().Add(rpc::Header::code, code);
			return this->Send(message->SockId(), message);
		}
		return XCode::Ok;
	}

	kcp::Session* OuterKcpComponent::GetSession()
	{
		std::string address;
		this->GetAddress(address);
		return this->GetSession(address);
	}

	kcp::Session* OuterKcpComponent::GetSession(const std::string& address)
	{
		std::shared_ptr<kcp::Session> session;
		auto iter = this->mClients.find(address);
		if(iter != this->mClients.end())
		{
			session = iter->second.lock();
		}
		else
		{
			int id = this->mNumberFactory.BuildNumber();
			session = std::make_shared<kcp::Session>(*this->mKcpServer, this->mRemotePoint, id);
			{
				this->mSessions.emplace(id, session);
				this->mClients.emplace(address, session);
			}
		}
		return session.get();
	}

	void OuterKcpComponent::Broadcast(std::unique_ptr<rpc::Message>& message) noexcept
	{
		auto iter = this->mSessions.begin();
		for(; iter != this->mSessions.end(); iter++)
		{
			std::unique_ptr<rpc::Message> request = message->Clone();
			{
				request->SetType(rpc::type::broadcast);
				iter->second->Send(request);
			}
		}
	}
}