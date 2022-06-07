﻿#pragma once

#include"Component/Component.h"
#include"Other/MultiThreadQueue.h"
#include"Async/RpcTask/RpcTaskSource.h"
namespace Sentry
{
	class ServiceComponent;

	class LocalLuaService;

	class ServiceMethod;

#ifdef __DEBUG__
	struct RpcTaskInfo
	{
		int MethodId;
		long long Time;
	};
#endif
	class RpcHandlerComponent : public Component, public IProtoRpc<com::Rpc_Request, com::Rpc_Response>
	{
	 public:
		RpcHandlerComponent() = default;
		~RpcHandlerComponent() final = default;
	 protected:
		void Awake() final;
		bool LateAwake() final;
	 public:
		XCode OnRequest(std::shared_ptr<com::Rpc_Request> request) final;
		XCode OnResponse(std::shared_ptr<com::Rpc_Response> response) final;
        void AddRpcTask(std::shared_ptr<IRpcTask<com::Rpc::Response>> task);
    private:
		void OnTaskTimeout(long long rpcId);
        std::shared_ptr<IRpcTask<com::Rpc::Response>> GetRpcTask(long long rpcId);
	 private:
		std::string mTempMethod;
		std::string mTempService;
		class TaskComponent* mTaskComponent;
		class TimerComponent* mTimerComponent;
		class RpcClientComponent* mRpcClientComponent;
		std::unordered_map<long long, std::shared_ptr<IRpcTask<com::Rpc::Response>>> mRpcTasks;
	};
}