﻿#pragma once

#include"Define/CommonTypeDef.h"
#include"Global/ServerConfig.h"
#include"Util/TimeHelper.h"
#include"Entity/Entity.h"
#include"Global/ServerPath.h"
#include"Thread/TaskThread.h"
#include"Global/RpcConfig.h"
#include"Component/Timer/TimerComponent.h"
#include"Component/Scene/LoggerComponent.h"
#include"Component/Coroutine/TaskComponent.h"
using namespace std;
using namespace asio::ip;

namespace Sentry
{
	class Manager;

	class RpcServiceNode;
	class MainTaskScheduler;
	class App : public Entity
	{
	 public:
		explicit App(ServerConfig* config);
		~App() final = default;
	 public:
		static std::shared_ptr<App> Get()
		{
			return mApp;
		}
		const ServerConfig& GetConfig()
		{
			return *mConfig;
		}
		const RpcConfig& GetRpcConfig()
		{
			return this->mRpcConfig;
		}
		inline MainTaskScheduler& GetTaskScheduler()
		{
			return this->mTaskScheduler;
		}

	 public:

		inline bool IsMainThread()
		{
			return std::this_thread::get_id() == this->mMainThreadId;
		}
		bool StartNewService(const std::string & name);
	 public:
		inline LoggerComponent* GetLogger()
		{
			return this->mLogComponent;
		}
		inline TaskComponent* GetTaskComponent()
		{
			return this->mTaskComponent;
		}
		inline TimerComponent* GetTimerComponent()
		{
			return this->mTimerComponent;
		}
	 private:
		bool LoadComponent();
		bool InitComponent(Component* component);
		void StartComponent(Component* component);
		bool AddComponentByName(const std::string& name);
	 protected:
		void OnAddComponent(Component *component) final;
		void OnDelComponent(Component *component) final;
	 public:
		int Run();
		void Stop();
	 private:
		void LogicMainLoop();
		bool StartNewComponent();
		void UpdateConsoleTitle();
		void WaitAllServiceStart();
	 private:
		bool mIsComplete;
		std::thread::id mMainThreadId;
		class MainTaskScheduler mTaskScheduler;
	 private:
		int mFps;
		float mDeltaTime;
		RpcConfig mRpcConfig;
		long long mStartTimer;
		long long mSecondTimer;
		long long mLogicUpdateInterval;
	 private:
		float mLogicFps;
		long long mStartTime;
		ServerConfig* mConfig;
		std::string mServerName;
		long long mLogicRunCount;
		long long mLastUpdateTime;
	 private:
		TaskComponent* mTaskComponent;
		LoggerComponent* mLogComponent;
		TimerComponent* mTimerComponent;
		static std::shared_ptr<App> mApp;
		std::queue<Component *> mNewComponents;
		std::vector<IFrameUpdate*> mFrameUpdateManagers;
		std::vector<ISystemUpdate*> mSystemUpdateManagers;
		std::vector<ISecondUpdate*> mSecondUpdateManagers;
		std::vector<ILastFrameUpdate*> mLastFrameUpdateManager;
	};
}// namespace Sentry