﻿#pragma once
#include<Manager/Manager.h>
#include<Other/TimeRecorder.h>
namespace SoEasy
{
	// 注册本地Lua服务，管理远程回来的回调
	class NetLuaAction;
	class LocalActionProxy;
	class LocalRetActionProxy;

	class ActionManager : public Manager
	{
	public:
		ActionManager();
		virtual ~ActionManager() { }
	public:
		long long AddCallback(shared_ptr<LocalRetActionProxy> rpcAction);
		bool InvokeCallback(long long id, PB::NetWorkPacket * messageData);
	protected:
		bool OnInit() override;
	private:
		int mMessageTimeout;
		std::string mMessageBuffer;
		TimeRecorder mLogicTimeRecorder;
		TimeRecorder mNetLatencyRecorder;
		class TimerManager * mTimerManager;
	private:
		std::unordered_map<long long, shared_ptr<LocalRetActionProxy>> mRetActionMap;
	};
}