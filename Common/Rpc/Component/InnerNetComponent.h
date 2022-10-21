﻿#pragma once

#include"Lua/Table.h"
#include"Client/Message.h"
#include"Client/InnerNetClient.h"
#include"Component/TcpListenerComponent.h"

namespace Sentry
{
	// 管理内网rpc的session
	class InnerNetComponent : public TcpListenerComponent, public IRpc<Rpc::Packet>
	{
	 public:
		InnerNetComponent() = default;
		~InnerNetComponent() override = default;
	 public:
		void StartClose(const std::string & address) final;
		void OnCloseSocket(const std::string & address, XCode code) final;
        void OnMessage(const std::string & address, std::shared_ptr<Rpc::Packet> message) final;
	 protected:
        bool Awake() final;
        bool LateAwake() final;
		bool OnListen(std::shared_ptr<SocketProxy> socket) final;
	 public:
        const std::string & GetUser() const { return this->mUserName; };
        const std::string & GetPassword() const { return this->mPassword; }
        const ServiceNodeInfo * GetSeverInfo(const std::string & address);
	 public:
        InnerNetClient * GetSession(const std::string& address);
        InnerNetClient * GetOrCreateSession(const std::string& address);
		bool Send(const std::string & address, std::shared_ptr<Rpc::Packet> message);
	private:
        bool IsAuth(const std::string & address);
        bool OnForward(std::shared_ptr<Rpc::Packet> message);
        bool OnBroadcast(std::shared_ptr<Rpc::Packet> message);
        bool OnAuth(const std::string & address, std::shared_ptr<Rpc::Packet> message);
        bool OnRequest(const std::string & address, std::shared_ptr<Rpc::Packet> message);
		bool OnResponse(const std::string& address, std::shared_ptr<Rpc::Packet> message);
	 private:
        std::string mUserName;
        std::string mPassword;
		std::string mLocation;
        class NetThreadComponent * mNetComponent;
        class OuterNetComponent * mOuterComponent;
        class InnerNetMessageComponent* mMessageComponent;
        std::unordered_map<std::string, std::string> mUserMaps;
        std::unordered_map<std::string, std::shared_ptr<InnerNetClient>> mRpcClientMap;
        std::unordered_map<std::string, std::unique_ptr<ServiceNodeInfo>> mLocationMaps;
    };
}
