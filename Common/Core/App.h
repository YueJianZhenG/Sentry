﻿#pragma once

#include"Define/CommonTypeDef.h"
#include"Global/ServerConfig.h"
#include"Util/TimeHelper.h"
#include"Object/GameObject.h"
#include"Global/ServerPath.h"
#include"Thread/TaskThread.h"
#include"Timer/TimerComponent.h"
#include"Scene/LoggerComponent.h"
#include"Coroutine/TaskComponent.h"
using namespace std;
using namespace asio::ip;


namespace GameKeeper
{
	enum ExitCode
	{
		Exit,
		AddError,
		InitError,
		StartError,
		ConfigError			
	};
}

namespace GameKeeper
{
	class Manager;

	class ServiceComponent;
	class MainTaskScheduler;
	class App : public GameObject
	{
	public:
		explicit App();
		~App() final = default;
	public:
		const ServerConfig &GetConfig()
		{
			return *mConfig;
		}

		inline float GetDelaTime() const
		{
			return this->mDelatime;
		}

		inline long long GetStartTime() const
		{
			return this->mStartTime;
		}

		const std::string &GetServerName()
		{
			return this->mServerName;
		}

        inline ServerPath & GetServerPath() const { return *mServerPath;}

		inline MainTaskScheduler & GetTaskScheduler() { return this->mTaskScheduler; }

		inline bool IsMainThread()
		{
			return std::this_thread::get_id() == this->mMainThreadId;
		}

	public:
		static App &Get()
		{
			return *mApp;
		}
	public:	
		inline LoggerComponent * GetLogger() { return this->mLogComponent; }
        inline TaskComponent * GetTaskComponent() { return this->mTaskComponent; }
        inline TimerComponent * GetTimerComponent() { return this->mTimerComponent; }
	private:
		
		bool InitComponent();

		bool AddComponentFormConfig();

		bool InitComponent(Component * component);

		void StartComponent();

	public:
		
		int Stop(ExitCode code);

		int Run(int argc, char ** argv);

	private:
		void UpdateConsoleTitle();
	private:
		void LogicMainLoop();
	private:
        ServerPath * mServerPath;
		std::thread::id mMainThreadId;
		class MainTaskScheduler mTaskScheduler;
	private:
		int mFps;
		long long mStartTimer;
		long long mSecondTimer;
		long long mLogicUpdateInterval;
	private:
		bool mIsClose;
		bool mIsInitComplate;
		long long mStartTime;
		ServerConfig * mConfig;
		std::string mServerName;
		long long mLastUpdateTime;
	private:
		float mLogicFps;
		float mDelatime;

	private:
		long long mLogicRunCount;
		long long mMainLoopStartTime;

	private:
		static App * mApp;
	private:
        TaskComponent * mTaskComponent;
        LoggerComponent * mLogComponent;
		TimerComponent * mTimerComponent;
		std::vector<Component *> mSceneComponents;
		std::vector<IFrameUpdate *> mFrameUpdateManagers;
		std::vector<ISystemUpdate *> mSystemUpdateManagers;
		std::vector<ISecondUpdate *> mSecondUpdateManagers;
		std::vector<ILastFrameUpdate *> mLastFrameUpdateManager;
    };
}// namespace GameKeeper