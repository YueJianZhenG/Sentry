//
// Created by 64658 on 2024/10/28.
//

#pragma once

#include "IClient.h"
namespace kcp
{
	class Session : public IClient
	{
	public:
		Session(asio_udp::socket & sock, asio_udp::endpoint & endpoint, int id);
		~Session() { ikcp_release(this->mKcp);}
	public:
		inline int GetSockId() const { return this->mSocketId; }
		inline const std::string & GetAddress() const { return this->mAddress; }
	public:
		bool Update(long long now) final;
		void Send(const char *buf, int len) final;
		void Send(std::unique_ptr<rpc::Message>& message) final;
		int Decode(const char * message, long len, char * buffer);
	private:
		ikcpcb * mKcp;
		int mSocketId;
		long long mLastTime;
		std::string mAddress;
		std::ostream mSendStream;
		asio_udp::socket & mSocket;
		asio_udp::endpoint mRemote;
		asio::streambuf mSendBuffer;
	};
}
