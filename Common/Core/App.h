﻿#pragma once

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif

#include <Define/CommonTypeDef.h>
#include <Global/ServerConfig.h>
#include <Util/TimeHelper.h>
#include <Object/GameObject.h>

#include <Timer/TimerComponent.h>
#include <Coroutine/CoroutineComponent.h>
using namespace std;
using namespace asio::ip;

namespace Sentry
{
	class Manager;

	class ServiceBase;

	class App
	{
	public:
		App(const std::string srvName, const std::string cfgDir);

		virtual ~App()
		{};

	public:
		ServerConfig &GetConfig()
		{
			return this->mConfig;
		}

		inline float GetDelaTime()
		{
			return this->mDelatime;
		}

		inline long long GetLogicTime()
		{
			return this->mLogicTime;
		}

		inline long long GetStartTime()
		{
			return this->mStartTime;
		}

		const std::string &GetServerName()
		{
			return this->mServerName;
		}

		inline AsioContext &GetNetContext()
		{
			return *mAsioContext;
		}

		long long GetRunTime()
		{
			return TimeHelper::GetMilTimestamp() - this->mStartTime;
		}

		inline const std::string &GetConfigDir()
		{
			return this->mConfigDir;
		}

		inline bool IsMainThread()
		{
			return std::this_thread::get_id() == this->mMainThreadId;
		}

		inline bool IsNetThreadThread()
		{
			return std::this_thread::get_id() == this->mNetWorkThreadId;
		}

	public:
		static App &Get()
		{
			return *mApp;
		}
	public:
		inline TimerComponent * GetTimerComponent() { return this->mTimerComponent; }
		inline CoroutineComponent * GetCoroutineComponent() { return this->mCoroutienComponent; }
	private:
		bool LoadComponent();

		bool InitComponent();

		bool StartNetThread();

		bool InitComponent(Component * component);

		void StartComponent();
	public:
		int Run();

		int Stop();

		float GetMeanFps();

	private:
		void UpdateConsoleTitle();
	private:
		int LogicMainLoop();

		void NetworkLoop();

	public:
		GameObject Scene;
		GameObject Service;
	private:
	    AsioWork * mAsioWork;
		AsioContext * mAsioContext;
		std::thread *mNetWorkThread;
		std::thread::id mMainThreadId;
		std::thread::id mNetWorkThreadId;
	private:
		bool mIsClose;
		bool mIsInitComplate;
		long long mLogicTime;
		long long mStartTime;
		ServerConfig mConfig;
		std::string mServerName;
		long long mLastUpdateTime;
		long long mLastSystemTime;
		class LogHelper *mLogHelper;
		
		std::string mConfigDir;

	private:
		float mLogicFps;
		float mDelatime;

	private:
		long long mLogicRunCount;
		long long mSystemRunCount;
		long long mMainLoopStartTime;

	private:
		static App * mApp;
	private:
		TimerComponent * mTimerComponent;
		CoroutineComponent * mCoroutienComponent;
		std::vector<Component *> mSceneComponents;
		std::vector<Component *> mServiceComponents;
		std::vector<IFrameUpdate *> mFrameUpdateManagers;
		std::vector<ISystemUpdate *> mSystemUpdateManagers;
		std::vector<ISecondUpdate *> mSecondUpdateManagers;
		std::vector<ILastFrameUpdate *> mLastFrameUpdateManager;
		std::vector<INetSystemUpdate *> mNetSystemUpdateManagers;
	};
}// namespace Sentry


namespace Sentry
{
	namespace Scene
	{
		template<typename T>
		inline T * GetComponent()
		{
			return App::Get().Scene.GetComponent<T>();
		}
	}

	namespace Service
	{
		template<typename T>
		inline T * GetComponent()
		{
			return App::Get().Service.GetComponent<T>();
		}
	}
}