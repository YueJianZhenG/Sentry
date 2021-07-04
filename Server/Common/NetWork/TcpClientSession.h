﻿#pragma once
#include<Define/CommonDef.h>
#include<Module/Component.h>
namespace SoEasy
{
	enum SessionState
	{
		Session_None,
		Session_Connect, //正在连接
		Session_Normal,	//正常
		Session_OK,	//可读可写
		Session_Error,	//错误
		Session_Close,	//手动关闭
	};


	struct NetWorkPack
	{
	public:
		NetWorkPack(std::string & ip, unsigned int port, char * buf, size_t size)
			: mIp(ip), mPort(port), mMessage(buf, size) {}
	public:
		const std::string mIp;
		const unsigned short mPort;
		const std::string mMessage;
	};
	class NetSessionManager;
	
	class TcpClientSession : public std::enable_shared_from_this<TcpClientSession>
	{
		typedef std::function<void(shared_ptr<TcpClientSession>, bool)> ConnectCallback;
	public:
		TcpClientSession(NetSessionManager * manager, SharedTcpSocket socket);
		TcpClientSession(NetSessionManager * manager, std::string name, std::string ip, unsigned short port);
		virtual ~TcpClientSession();
	public:
		bool IsActive();	
		inline const std::string & GetIP() { return mIp; }
		inline unsigned short GetPort() { return mPort; }
		inline SessionState GetState() { return mCurrentSatte; }
		inline const bool IsContent() { return this->mIsContent; }
		inline const std::string & GetAddress() { return mAdress; }	
		inline SharedTcpSocket GetSocket() { return this->mBinTcpSocket; }
		inline const std::string & GetSessionName() { return mSessionName; }
	public:		
		bool SendPackage(const shared_ptr<std::string> message);
	public:
		void StartClose();
		bool StartConnect();
		void StartReceiveMsg();
	private:
		void ReadMessageBody(const  size_t allSize);
		void InitMember(const std::string & ip, unsigned short port);
	private:
		std::string mIp;
		std::string mAdress;
		unsigned short mPort;
		AsioContext & mAsioContext;
		SharedTcpSocket mBinTcpSocket;
		AsioTcpEndPoint mSocketEndPoint;
	private:
		bool mIsContent;
		std::string mSessionName;
		SessionState mCurrentSatte;
		unsigned int mConnectCount;
		NetSessionManager * mDispatchManager;
	private:	
		std::function<void()> mRecvAction;
	private:
		char * mRecvMsgBuffer;
		unsigned int mRecvBufferSize;
	};
	typedef shared_ptr<TcpClientSession> SharedTcpSession;
	
}