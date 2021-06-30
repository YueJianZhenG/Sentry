﻿#pragma once

#include<Protocol/com.pb.h>
#include<Manager/Manager.h>
#include<Core/TcpSessionListener.h>
#include<Script/LuaTable.h>
#include<Protocol/s2s.pb.h>

namespace SoEasy
{

	class TcpClientSession;
	class NetWorkManager : public Manager
	{
	public:
		NetWorkManager();
		virtual ~NetWorkManager() {}
	public:
		friend Applocation;
	public:
		bool AddTcpSession(shared_ptr<TcpClientSession> tcpSession);
		bool CloseTcpSession(const std::string & address);
		bool CloseTcpSession(shared_ptr<TcpClientSession> tcpSession);

		bool RemoveTcpSession(const std::string & tcpSession);
		bool RemoveTcpSession(shared_ptr<TcpClientSession> tcpSession);
		bool SendMessageByAdress(const std::string & address, const SharedPacket & returnPackage);
	public:
		shared_ptr<TcpClientSession> GetTcpSession(const std::string & adress);
	protected:
		bool OnInit() override;
		void OnDestory() override;
	private:
		std::mutex mSessionLock;
		AsioContext * mSessionContext;
		class ActionManager * mLocalActionManager;
	private:
		char mSendSharedBuffer[ASIO_TCP_SEND_MAX_COUNT + sizeof(unsigned int)];
		std::unordered_map<std::string, shared_ptr<TcpClientSession>> mSessionAdressMap;	//所有session
	};
}
