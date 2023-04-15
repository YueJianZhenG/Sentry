#pragma once
#include<mutex>
#include"Asio.h"
#include"Log/Common/CommonLogDef.h"

namespace Tcp
{
	class SocketProxy
	{
	 public:
		explicit SocketProxy(Asio::Context & io, const std::string & net);
		~SocketProxy();
	 public:
        void Init();
        void Init(const std::string & ip, unsigned short port);
    public:
        void Close();
		unsigned short GetPort() const { return this->mPort;}
		inline std::string & GetIp() { return this->mIp;}
		inline Asio::Socket & GetSocket() { return *mSocket;}
		inline Asio::Context & GetThread() { return this->mNetThread; }
		inline const std::string& GetAddress() { return this->mAddress; }
	 private:
		std::string mIp;
		std::string mNet;
		unsigned short mPort;
		std::string mAddress;
        Asio::Socket * mSocket;
        Asio::Context & mNetThread;
    };
}

namespace Tendo
{
	enum DataMessageType
	{
		TYPE_REQUEST = 0X01,        //服务器请求
		TYPE_RESPONSE = 0X02,       //服务器返回
        TYPE_CLIENT_REQUEST = 0X03, //客户端请求
	};
	enum class SocketType
	{
		NoneSocket,
		LocalSocket,
		RemoteSocket
	};
}