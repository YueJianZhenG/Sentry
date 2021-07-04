﻿#pragma once
#include<thread>
#include<Protocol/com.pb.h>
#include<Manager/Manager.h>

#include<Script/LuaTable.h>
#include<Protocol/s2s.pb.h>
#include<NetWork/SocketEvent.h>
#include<Other/DoubleBufferQueue.h>
#include<Pool/ObjectPool.h>
namespace SoEasy
{
	// 管理所有session  在网络线程中运行
	class TcpClientSession;
	class NetSessionManager : public Manager
	{
	public:
		NetSessionManager();
		virtual ~NetSessionManager() {}
	public:
		friend Applocation;
	public: //网络线程调用
		void OnConnectSuccess(SharedTcpSession session);
		void OnSessionError(SharedTcpSession session, Net2MainEventType type);
		bool OnRecvMessage(SharedTcpSession session, const char * message, const size_t size);
		bool OnSendMessageError(SharedTcpSession session, const char * message, const size_t size);
	public:
		bool AddNetSessionEvent(Main2NetEvent * eve);
	public:
		shared_ptr<TcpClientSession> Create(shared_ptr<AsioTcpSocket> socket);
		shared_ptr<TcpClientSession> Create(const std::string & name, const std::string & address);
	public:
		AsioContext & GetAsioCtx() { return (mAsioContext); }
	protected:
		bool OnInit() override;
		void OnDestory() override;
		void OnInitComplete() final;				//在初始化完成之后 改方法会在协程中调用
	private:
		void NetUpdate();
		bool IsInNetThead();
		void HandlerMainThreadEvent(Main2NetEvent * eve);
	private:
		bool DescorySession(const std::string & address);
		shared_ptr<TcpClientSession> GetSession(const std::string & address);
	private:
		AsioContext mAsioContext;
		class ActionManager * mLocalActionManager;
		class NetProxyManager * mNetProxyManager;
	private:
		bool mIsClose;
		std::thread * mNetThread;
		std::thread::id mNetThreadId;
		class ListenerManager * mListenerManager;
		DoubleBufferQueue<Main2NetEvent *> mNetEventQueue;
		char mSendSharedBuffer[ASIO_TCP_SEND_MAX_COUNT + sizeof(unsigned int)];
		std::unordered_map<std::string, shared_ptr<TcpClientSession>> mSessionAdressMap;	//所有session
	};
}
