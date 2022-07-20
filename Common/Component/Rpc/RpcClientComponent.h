﻿#pragma once

#include"Component/Component.h"
#include"Script/Table.h"
#include"Other/MultiThreadQueue.h"
#include"Network/Rpc/ServerClientContext.h"
namespace Sentry
{
	// 管理所有session
	class RpcClientComponent : public Component, public ISocketListen,
							   public IRpc<com::Rpc_Request, com::Rpc_Response>
	{
	 public:
		RpcClientComponent() = default;
		~RpcClientComponent() override = default;
	 public:
		void StartClose(const std::string & address) final;
		void OnCloseSocket(const std::string & address, XCode code) final;
		void OnRequest(std::shared_ptr<com::Rpc_Request> request) final;
		void OnResponse(std::shared_ptr<com::Rpc_Response> response) final;

	 protected:
		void OnListen(std::shared_ptr<SocketProxy> socket) final;
	 public:
		std::shared_ptr<ServerClientContext> GetSession(const std::string& address);
		std::shared_ptr<ServerClientContext> GetOrCreateSession(const std::string& address);
	 protected:
		void Awake() final;
		bool LateAwake() final;
	 public:
		bool CloseSession(long long id);
		bool Send(const std::string & address, std::shared_ptr<com::Rpc_Request> message);
		bool Send(const std::string & address, std::shared_ptr<com::Rpc_Response> message);
	 private:
		class ServiceRpcComponent* mRpcComponent;
		class NetThreadComponent* mTaskComponent;
		std::unordered_map<std::string, std::shared_ptr<ServerClientContext>> mRpcClientMap;
	};
}
