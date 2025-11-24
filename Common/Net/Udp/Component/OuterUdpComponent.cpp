//
// Created by 64658 on 2024/10/24.
//

#include "XCode/XCode.h"
#include "Util/Tools/String.h"
#include "Message/c2s/c2s.pb.h"
#include "OuterUdpComponent.h"
#include "Rpc/Common/Message.h"
#include "Entity/Actor/App.h"
#include "Server/Config/CodeConfig.h"
#include "Server/Component/ThreadComponent.h"
#include "Rpc/Component/DispatchComponent.h"
#include "Util/Tools/TimeHelper.h"

namespace acs
{
	OuterUdpComponent::OuterUdpComponent()
		: mReadStream(&mReceiveBuffer)
	{
		this->mDispatch = nullptr;
		this->mMsg = rpc::msg::text;
	}

	bool OuterUdpComponent::LateAwake()
	{
		this->mDispatch = this->GetComponent<DispatchComponent>();
		return true;
	}

	bool OuterUdpComponent::StopListen()
	{
		Asio::Code code;
		code = this->mSocket->close(code);
		return code.value() == Asio::OK;
	}

	bool OuterUdpComponent::StartListen(const acs::ListenConfig& listen)
	{
		try
		{
			Asio::Context& context = this->mApp->GetContext();
			{
				unsigned short port = listen.port;
				udp::EndPoint endpoint(asio::ip::udp::v4(), port);
				this->mSocket = std::make_unique<udp::Socket>(context, endpoint);
			}
			asio::post(context, [this]() { this->StartReceive(); });
			return true;
		}
		catch (std::exception& e)
		{
			return false;
		}
	}

	void OuterUdpComponent::StartReceive()
	{
		auto callback = [this](const asio::error_code& code, size_t size)
		{
			if (code.value() == Asio::OK)
			{
				this->OnReceiveMessage(size);
			}
			if (code != asio::error::operation_aborted)
			{
				this->StartReceive();
			}
		};
		auto buffer = this->mReceiveBuffer.prepare(udp::BUFFER_COUNT);
		this->mSocket->async_receive_from(buffer, this->mRemotePoint, callback);
	}

	void OuterUdpComponent::GetAddress(std::string& address)
	{
		std::string ip = this->mRemotePoint.address().to_string();
		address = fmt::format("{}:{}", ip, this->mRemotePoint.port());
	}

	udp::Session* OuterUdpComponent::GetSession()
	{
		std::string address;
		this->GetAddress(address);
		return this->GetSession(address);
	}

	udp::Session* OuterUdpComponent::GetSession(const std::string& address)
	{
		std::shared_ptr<udp::Session> session;
		auto iter = this->mSessionClients.find(address);
		if(iter != this->mSessionClients.end())
		{
			session = iter->second.lock();
		}
		else
		{
			int id = this->mNumPool.BuildNumber();
			session = std::make_shared<udp::Session>(*this->mSocket, this->mRemotePoint, id);
			{
				this->mSessions.emplace(id, session);
				this->mSessionClients.emplace(address, session);
			}
		}
		return session.get();
	}

	void OuterUdpComponent::OnReceiveMessage(size_t size)
	{
		this->mReceiveBuffer.commit(size);
		udp::Session * udpSession = this->GetSession();
		std::unique_ptr<rpc::Message> rpcPacket = std::make_unique<rpc::Message>();
		{
			rpcPacket->SetMsg(this->mMsg);
			rpcPacket->SetNet(rpc::net::udp);
			rpcPacket->SetSockId(udpSession->GetSockId());
			if (rpcPacket->OnRecvMessage(this->mReadStream, size) != tcp::read::done)
			{
				return;
			}
			Asio::Context& context = this->mApp->GetContext();
			asio::post(context, [this, req = rpcPacket.release()]{ this->OnMessage(req, nullptr); });
		}
		this->mReceiveBuffer.consume(size);
	}

	int OuterUdpComponent::Send(int id, std::unique_ptr<rpc::Message>& message) noexcept
	{
		message->SetMsg(this->mMsg);
		auto iter = this->mSessions.find(id);
		if(iter == this->mSessions.end())
		{
			return XCode::SendMessageFail;
		}
		iter->second->Send(message);
		return XCode::Ok;
	}

	void OuterUdpComponent::OnMessage(rpc::Message* request, rpc::Message* ) noexcept
	{
		std::unique_ptr<rpc::Message> message(request);
		if(request->GetType() == rpc::type::request)
		{
			this->OnRequest(message);
		}
	}

	int OuterUdpComponent::OnRequest(std::unique_ptr<rpc::Message>& message) noexcept
	{
		int code = this->mDispatch->OnMessage(message);
		if (code != XCode::Ok)
		{
			const std::string& desc = CodeConfig::Inst()->GetDesc(code);
			LOG_ERROR("call {} code = {}", message->GetHead().GetStr(rpc::Header::func), desc);

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

	void OuterUdpComponent::Broadcast(std::unique_ptr<rpc::Message>& message) noexcept
	{

	}
}
