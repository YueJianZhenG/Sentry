//
// Created by 64658 on 2024/11/19.
//

#ifndef APP_ITCPCOMPONENT_H
#define APP_ITCPCOMPONENT_H
#include <string>
#include"Network/Tcp/Socket.h"

namespace acs
{

	class ITcpListen
	{
	public:
		virtual ~ITcpListen() = default;
		virtual bool OnListen(tcp::Socket* socket) noexcept = 0;
	};

	namespace proto_type
	{
		constexpr int tcp = 1;
		constexpr int udp = 2;
		constexpr int local = 3;
	}

	struct ListenConfig
	{
	public:
		std::string ip;
		int max_conn = 0;
		unsigned short port = 0;            //监听
		int proto = proto_type::tcp;
#ifdef __ENABLE_OPEN_SSL__
		std::string key;    //私钥
		std::string cert;    //证书地址 .pem或者.crt格式
		std::string verify;
#endif
		std::string name;    //名字
		std::string address;  //地址 xxx://xxx:xxx
		std::string component; //处理消息的component
		std::string proto_name;
	};

	class INetListen
	{
	public:
		virtual ~INetListen() = default;
		virtual bool StopListen() = 0;
		virtual bool StartListen(const ListenConfig & listen) = 0;
	};
}

#endif //APP_ITCPCOMPONENT_H
