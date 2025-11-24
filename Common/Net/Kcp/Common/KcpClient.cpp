//
// Created by 64658 on 2024/10/24.
//

#include "KcpClient.h"
#include <sstream>
#include "Util/Tools/String.h"
#include "Log/Common/CommonLogDef.h"
#include "Util/Tools/TimeHelper.h"
namespace kcp
{
	Client::Client(asio::io_context& io,
			kcp::Client::Component* component, asio_udp::endpoint & remote)
			: mComponent(component), mSocket(io, asio_udp::endpoint(asio_udp::v4(), 0)),
			 mEndpoint(remote), mTimer(io), mSendStream(&mSendBuffer)
	{
		this->mLastRecvTime = 0;
		this->mKcp = ikcp_create(0x01, this);
		this->mKcp->output = kcp::OnKcpSend;
		ikcp_wndsize(this->mKcp, kcp::BUFFER_COUNT, kcp::BUFFER_COUNT);
		ikcp_nodelay(this->mKcp, 1, kcp::REFRESH_INTERVAL, kcp::RESEND, 1);
	}

	void Client::Send(const char* buf, int len)
	{
//		std::shared_ptr<Client> self = this->shared_from_this();
//		this->mSocket.async_send(asio::buffer(buf, len), [self](const asio::error_code & code, size_t)
//		{
//			if(code.value() != Asio::OK)
//			{
//				unsigned short port = self->mLocalEndpoint.port();
//				const std::string ip = self->mLocalEndpoint.address().to_string();
//				LOG_ERROR("kcp send [{}:{}] =>{}", ip, port, code.message());
//			}
//		});
		asio::error_code code;
		asio::socket_base::message_flags flags = 0;
		this->mSocket.send_to(asio::buffer(buf, len), this->mEndpoint, flags, code);
	}

	void Client::Send(std::unique_ptr<rpc::Message> & message)
	{
		message->OnSendMessage(this->mSendStream);
		const int len = (int)this->mSendBuffer.size();
		const char* msg = asio::buffer_cast<const char*>(this->mSendBuffer.data());
		{
			ikcp_send(this->mKcp, msg, len);
			this->mSendBuffer.consume(len);
		}
	}

	bool Client::Update(long long now)
	{
		if(now - this->mLastRecvTime >= kcp::TIME_OUT_MS)
		{
			return false;
		}
		ikcp_update(this->mKcp, (IUINT32)now);
		return true;
	}

	void Client::StartReceive()
	{
		this->mLastRecvTime = help::Time::NowSec() * 1000;
		std::shared_ptr<Client> self = this->shared_from_this();
		auto callback = [self](const asio::error_code& code, size_t size)
		{
			if (code.value() != Asio::OK)
			{
				CONSOLE_LOG_ERROR("code:{}", code.message())
				return;
			}
			self->mReceiveBuffer.commit(size);
			unsigned short port = self->mLocalEndpoint.port();
			std::string ip = self->mLocalEndpoint.address().to_string();

			const char* msg = asio::buffer_cast<const char*>(self->mReceiveBuffer.data());

			ikcp_input(self->mKcp, msg, (int)size);
			self->mDecodeBuffer.resize(kcp::BUFFER_COUNT);
			char * buffer = (char *)self->mDecodeBuffer.data();
			int messageLen = ikcp_recv(self->mKcp, buffer, kcp::BUFFER_COUNT);
			if (messageLen > 0)
			{
				const std::string address = fmt::format("{}:{}", ip, port);
				self->OnReceive(address, self->mDecodeBuffer, messageLen);
			}
			self->mReceiveBuffer.consume(size);
			self->StartReceive();
		};
		auto buffer = this->mReceiveBuffer.prepare(kcp::BUFFER_COUNT);
		this->mSocket.async_receive_from(buffer, this->mLocalEndpoint, callback);
	}

	void Client::OnReceive(const std::string& address, const std::string & buf, size_t size)
	{
		std::stringstream buffer(buf);
		std::unique_ptr<rpc::Message> rpcPacket = std::make_unique<rpc::Message>();
		{
			rpcPacket->SetMsg(rpc::msg::bin);
			rpc::Head & head = rpcPacket->GetHead();
			tcp::Data::ReadHead(buffer, rpcPacket->GetProtoHead(), true);
			if(rpcPacket->OnRecvMessage(buffer, size) == 0)
			{
				rpcPacket->SetNet(rpc::net::kcp);
				head.Add(rpc::Header::from_addr, address);
				this->mComponent->OnMessage(rpcPacket.release(), nullptr);
			}
		}
	}
}