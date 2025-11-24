//
// Created by 64658 on 2025/9/15.
//

#include "UdpSession.h"

namespace udp
{
	Session::Session(asio::ip::udp::socket& sock, asio::ip::udp::endpoint& endpoint, int id)
		: mSocket(sock), mEndpoint(endpoint), mSocketId(id), mStream(&mSendBuffer)
	{

	}

	void Session::Send(std::unique_ptr<rpc::Message>& message)
	{
		asio::error_code code;
		message->OnSendMessage(this->mStream);
		asio::socket_base::message_flags flags = 0;
		size_t count = this->mSocket.send_to(this->mSendBuffer.data(), this->mEndpoint, flags, code);
		if(code.value() != 0)
		{
			return;
		}
		//LOG_DEBUG("send size => {}:{}", count, this->mSendBuffer.size())
		this->mSendBuffer.consume(count);
	}
}