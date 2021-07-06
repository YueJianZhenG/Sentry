﻿#pragma once
#include <XCode/XCode.h>
#include <Protocol/com.pb.h>
#include <NetWork/SocketEvent.h>

using namespace google::protobuf;
namespace SoEasy
{
	class TcpProxySession
	{
	public:
		TcpProxySession(const std::string &address);
		TcpProxySession(const std::string &name, const std::string &address);
		~TcpProxySession();

	public:
		const std::string &GetName() { return this->mName; }
		const std::string &GetAddress() { return this->mAddress; }
		bool IsNodeSession() { return this->mSessionType == SessionType::SessionNode; }
		int GetConnectCount() { return this->mConnectCount}
	public:
		bool StartColse();
		bool StartReceive();
		void StartConnect();
		bool SendMessageData(PB::NetWorkPacket *messageData);

	public:
		bool Notice(const std::string &service, const std::string &method);							//不回应
		bool Notice(const std::string &service, const std::string &method, const Message &request); //不回应
	private:
		int mConnectCount;
		std::string mName;
		std::string mAddress;
		SessionType mSessionType;
		std::string mMessageBuffer;
		class CoroutineManager *mCorManager;
		class NetSessionManager *mNetManager;
		class NetProxyManager *mNetProxyManager;
	};
}