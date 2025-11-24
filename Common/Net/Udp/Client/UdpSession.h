//
// Created by 64658 on 2025/9/15.
//

#ifndef APP_UDPSESSION_H
#define APP_UDPSESSION_H
#include <memory>
#include <asio/ip/udp.hpp>
#include <asio/streambuf.hpp>
#include "Rpc/Common/Message.h"
namespace udp
{
	class Session : public std::enable_shared_from_this<Session>
	{
	public:
		Session(asio::ip::udp::socket & sock, asio::ip::udp::endpoint & endpoint, int id);
	public:
		void Send(std::unique_ptr<rpc::Message> & message);
		inline int GetSockId() const { return this->mSocketId; }
	private:
		int mSocketId;
		std::ostream mStream;
		asio::streambuf mSendBuffer;
		asio::ip::udp::socket & mSocket;
		asio::ip::udp::endpoint mEndpoint;
	};
}
#endif //APP_UDPSESSION_H
