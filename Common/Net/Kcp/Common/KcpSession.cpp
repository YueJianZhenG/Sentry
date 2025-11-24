//
// Created by 64658 on 2024/10/28.
//

#include "KcpSession.h"
#include "Util/Tools/TimeHelper.h"
#include "Log/Common/CommonLogDef.h"
namespace kcp
{
	Session::Session(asio_udp::socket& sock, asio_udp::endpoint& endpoint, int id)
			: mSocket(sock), mRemote(endpoint), mSendStream(&mSendBuffer), mSocketId(id)
	{
		this->mKcp = ikcp_create(0x01, this);
		this->mKcp->output = kcp::OnKcpSend;
		this->mLastTime = help::Time::NowSec();
		std::string ip = endpoint.address().to_string();
		ikcp_wndsize(this->mKcp, 128, 128);
		ikcp_nodelay(this->mKcp, 1, kcp::REFRESH_INTERVAL, kcp::RESEND, 1);
		this->mAddress = fmt::format("{}:{}", ip, endpoint.port());
	}

	void Session::Send(const char* buf, int len)
	{
		asio::error_code code;
		//printf("send count => %d\n", len);
		asio::socket_base::message_flags flags = 0;
		this->mSocket.send_to(asio::buffer(buf, len), this->mRemote, flags, code);
	}

	int Session::Decode(const char* message, long len, char * buffer)
	{
		ikcp_input(this->mKcp, message, len);
		this->mLastTime = help::Time::NowMil();
		return ikcp_recv(this->mKcp, buffer, kcp::BUFFER_COUNT);
	}

	void Session::Send(std::unique_ptr<rpc::Message> & message)
	{
		message->OnSendMessage(this->mSendStream);
		int len = (int)this->mSendBuffer.size();
		const char* msg = asio::buffer_cast<const char*>(this->mSendBuffer.data());
		{
			ikcp_send(this->mKcp, msg, len);
		}
		ikcp_flush(this->mKcp);
		this->mSendBuffer.consume(len);
	}

	bool Session::Update(long long now)
	{
		if(now - this->mLastTime >= kcp::TIME_OUT_MS)
		{
			return false;
		}
		ikcp_update(this->mKcp, (IUINT32)now);
		return true;
	}
}