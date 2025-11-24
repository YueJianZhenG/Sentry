//
// Created by 64658 on 2025/1/16.
//

#ifndef APP_LUATCPSOCKET_H
#define APP_LUATCPSOCKET_H
#include "Network/Tcp/Client.h"

namespace lua
{
	class TcpClient
	{
	public:
		TcpClient() = default;
		~TcpClient() { printf("------------------\n");}
	public:
		int timeout = 0;
		bool async = true; //是不是异步模式
		asio::streambuf mBuffer;
		asio::streambuf mSendBuffer;
		std::unique_ptr<Asio::Timer> mTimer;
		std::unique_ptr<tcp::Socket> mSocket;
	};
}
#endif //APP_LUATCPSOCKET_H
