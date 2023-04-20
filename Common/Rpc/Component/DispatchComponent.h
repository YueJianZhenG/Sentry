﻿#pragma once
#include"RpcTaskComponent.h"
#include"Rpc/Client/Message.h"
#include"Rpc/Async/RpcTaskSource.h"
#include"Server/Config/MethodConfig.h"

namespace Tendo
{
	class RpcService;

	class LuaPhysicalRpcService;

	class ServiceMethod;

#ifdef __DEBUG__
	struct RpcTaskInfo
	{
		int MethodId;
		long long Time;
	};
#endif
    class DispatchComponent : public RpcTaskComponent<int, Msg::Packet>
	{
	 public:
		DispatchComponent();
    public:
		int OnMessage(const std::shared_ptr<Msg::Packet>& message);
		unsigned int WaitCount() const { return this->mWaitCount; }
    private:
		bool LateAwake() final;
		int OnPublishMessage(const std::shared_ptr<Msg::Packet>& message);
		int OnForwardMessage(const std::shared_ptr<Msg::Packet>& message);
		int OnRequestMessage(const std::shared_ptr<Msg::Packet> & message);
		int OnResponseMessage(const std::shared_ptr<Msg::Packet>& message);
		int OnBroadcastMessage(const std::shared_ptr<Msg::Packet>& message);
		void Invoke(const RpcMethodConfig * config, const std::shared_ptr<Msg::Packet>& message);
    private:
		unsigned int mWaitCount;
		class InnerRpcComponent * mNetComponent;
		class CoroutineComponent* mTaskComponent;
		class TimerComponent* mTimerComponent;
		class OuterNetComponent* mOuterComponent;
	};
}