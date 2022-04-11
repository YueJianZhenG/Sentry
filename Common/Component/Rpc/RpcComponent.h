﻿#pragma once

#include"Protocol/s2s.pb.h"
#include"Component/Component.h"
#include"Other/MultiThreadQueue.h"
namespace Sentry
{
	class RpcServiceNode;

	class LuaRpcService;

	class ServiceMethod;

#ifdef __DEBUG__
	struct RpcTaskInfo
	{
		int MethodId;
		long long Time;
	};
#endif

	class IRpcTask;
	class RpcComponent : public Component, public IProtoRpc<com::Rpc_Request, com::Rpc_Response>
	{
	 public:
		RpcComponent() = default;
		~RpcComponent() final = default;

	 public:
		void AddRpcTask(std::shared_ptr<IRpcTask> task);
#ifdef __DEBUG__
		void AddRpcInfo(long long rpcId, int methodId);
		bool GetRpcInfo(long long rpcId, int& methodId, long long& time);
#endif
	 protected:
		void Awake() final;
		bool LateAwake() final;
	 public:
		XCode OnRequest(std::shared_ptr<com::Rpc_Request> request) final;
		XCode OnResponse(std::shared_ptr<com::Rpc_Response> response) final;
	 private:
		void OnTaskTimeout(long long rpcId);
	 private:
		class TaskComponent* mCorComponent;
		class TimerComponent* mTimerComponent;
		class RpcClientComponent* mRpcClientComponent;
#ifdef __DEBUG__
		std::unordered_map<long long, RpcTaskInfo> mRpcInfoMap;
#endif
		std::unordered_map<long long, std::shared_ptr<IRpcTask>> mRpcTasks;
	};
}