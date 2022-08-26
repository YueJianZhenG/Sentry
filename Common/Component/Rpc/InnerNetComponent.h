﻿#pragma once

#include"Component/Component.h"
#include"Script/Table.h"
#include"Network/Rpc/InnerNetClient.h"
#include"Network/Listener/TcpServerListener.h"
namespace Sentry
{
	// 管理内网rpc的session
	class InnerNetComponent : public Component, public TcpServerListener,
                              public IRpc<Tcp::RpcMessage>
	{
	 public:
		InnerNetComponent() = default;
		~InnerNetComponent() override = default;
	 public:
		void StartClose(const std::string & address) final;
		void OnCloseSocket(const std::string & address, XCode code) final;
        void OnMessage(const std::string & address, std::shared_ptr<Tcp::RpcMessage> message) final;
	 protected:
        void Awake() final;
        bool LateAwake() final;
		bool OnListen(std::shared_ptr<SocketProxy> socket) final;
	 public:
		std::shared_ptr<InnerNetClient> GetSession(const std::string& address);
		std::shared_ptr<InnerNetClient> GetOrCreateSession(const std::string& address);
	 public:
		bool Send(const std::string & address, std::shared_ptr<com::rpc::request> message);
		bool Send(const std::string & address, std::shared_ptr<com::rpc::response> message);
	 private:
        class NetThreadComponent * mNetComponent;
        class InnerNetMessageComponent* mMessageComponent;
        std::unordered_map<std::string, std::shared_ptr<InnerNetClient>> mRpcClientMap;
	};
}