﻿
#include "App.h"
#include <Scene/CallHandlerComponent.h>
#include <Scene/ProtocolComponent.h>
#include <Network/Tcp/TcpClientComponent.h>
#include <Service/ServiceNodeComponent.h>
#include <Service/ServiceMgrComponent.h>
#include <Scene/TaskPoolComponent.h>
#include<Util/DirectoryHelper.h>
using namespace Sentry;
using namespace std::chrono;

namespace Sentry
{
	App *App::mApp = nullptr;

	App::App(int argc, char ** argv)
		:GameObject(0), mServerPath(argc, argv),
		mStartTime(TimeHelper::GetMilTimestamp()),
		mTaskScheduler(NewMethodProxy(&App::LogicMainLoop, this))
	{
		mApp = this;
		this->mDelatime = 0;
		this->mIsClose = false;
		this->mLogicRunCount = 0;
		this->mIsInitComplate = false;
        this->mMainThreadId = std::this_thread::get_id();
		this->mServerName = argc == 1 ? "server" : argv[1];
        this->mNextRefreshTime = TimeHelper::GetTomorrowZeroTime() * 1000;
		LogHelper::Init(this->mServerPath.GetLogPath(), this->mServerName);
		this->mConfig = new ServerConfig(this->mServerPath.GetConfigPath() + this->mServerName + ".json");
	}

	void App::OnZeroRefresh()
	{
		spdlog::drop_all();
		LogHelper::Init("./Logs", this->mServerName);
		for (Component * component : this->mSceneComponents)

		{
			if (auto zeroComponent = dynamic_cast<IZeroRefresh*>(component))
			{
				zeroComponent->OnZeroRefresh();
			}
		}
	}

	bool App::LoadComponent()
	{
		this->AddComponent<TimerComponent>();
		this->AddComponent<CallHandlerComponent>();
		this->AddComponent<ProtocolComponent>();
		this->AddComponent<CoroutineComponent>();
		this->AddComponent<TcpClientComponent>();

		this->AddComponent<ServiceNodeComponent>();
		this->AddComponent<ServiceMgrComponent>();

		this->mTimerComponent = this->GetComponent<TimerComponent>();
		this->mCorComponent = this->GetComponent<CoroutineComponent>();



		std::vector<std::string> services;
		std::vector<std::string> components;
		if (!mConfig->GetValue("Scene", components))
		{
			SayNoDebugError("not find field : Managers");
			return false;
		}

		if (!mConfig->GetValue("Service", services))
		{
			SayNoDebugError("not find field : Service");
			return false;
		}

		for (const std::string & name : components)
		{
			if (!this->AddComponent(name))
			{
				SayNoDebugFatal("add " << name << " to service failure");
				return false;
			}
		}

		for (const std::string & name : services)
		{
			if (!this->AddComponent(name))
			{
				SayNoDebugFatal("add " << name << " to scene failure");
				return false;
			}
		}
		return true;
	}

	bool App::InitComponent()
	{

		// 初始化scene组件
		this->GetComponents(this->mSceneComponents);
		std::sort(mSceneComponents.begin(), mSceneComponents.end(),
			[](Component *m1, Component *m2) -> bool
		{
			return m1->GetPriority() < m2->GetPriority();
		});

		for (Component *component : mSceneComponents)
		{
			if (!this->InitComponent(component))
			{
				SayNoDebugFatal("Init " << component->GetTypeName() << " failure");
				return false;
			}
		}
		this->mCorComponent->StartCoroutine(&App::StartComponent, this);
		return true;
	}

	bool App::InitComponent(Component * component)
	{
		if (!component->IsActive() || !component->Awake())
		{
			return false;
		}

		if (auto manager1 = dynamic_cast<IFrameUpdate *>(component))
		{
			this->mFrameUpdateManagers.push_back(manager1);
		}
		if (auto manager2 = dynamic_cast<ISystemUpdate *>(component))
		{
			this->mSystemUpdateManagers.push_back(manager2);
		}
		if (auto *manager3 = dynamic_cast<ISecondUpdate *>(component))
		{
			this->mSecondUpdateManagers.push_back(manager3);
		}
		return true;
	}

	void App::StartComponent()
	{
		for (int index = 0; index < this->mSceneComponents.size(); index++)
		{
			Component *component = this->mSceneComponents[index];
			if (component != nullptr)
			{
				float process = index / (float)this->mSceneComponents.size();
				SayNoDebugInfo("[" << process * 100 << "%]"
					<< " start component " << component->GetTypeName());
				component->Start();
			}
		}
		this->mIsInitComplate = true;
		this->mMainLoopStartTime = TimeHelper::GetMilTimestamp();
		SayNoDebugLog("start all scene component successful ......");
		long long t = TimeHelper::GetMilTimestamp() - this->mStartTime;

		for (Component *component : this->mSceneComponents)
		{
			if (auto loadComponent = dynamic_cast<ILoadData *>(component))
			{
				loadComponent->OnLodaData();
				SayNoDebugLog("load " << component->GetTypeName() << " data");
			}
		}
		SayNoDebugLog("=====  start " << this->mServerName << " successful [" << t / 1000.0f << "s] ========");
	}

	int App::Run()
	{
		if (!mConfig->InitConfig() || !this->LoadComponent() || !this->InitComponent())
		{
			return this->Stop();
		}
		mConfig->GetValue("LogicFps", this->mFps);
		this->mLogicUpdateInterval = 1000 / this->mFps;
		this->mStartTime = TimeHelper::GetMilTimestamp();
		this->mSecondTimer = TimeHelper::GetMilTimestamp();
		this->mLastUpdateTime = TimeHelper::GetMilTimestamp();

		return this->mTaskScheduler.Run();
	}

	int App::Stop()
	{
		this->OnDestory();
		this->mIsClose = true;
		spdlog::drop_all();
#ifdef _WIN32
		return getchar();
#endif
		return -1;
	}

	void App::Hotfix()
	{
		for (Component * component : this->mSceneComponents)
		{
			if (auto hotfix = dynamic_cast<IHotfix*>(component))
			{
				hotfix->OnHotFix();
			}
		}
	}

	void App::LogicMainLoop()
	{
		this->mStartTimer = TimeHelper::GetMilTimestamp();
		for (ISystemUpdate *component : this->mSystemUpdateManagers)
		{
			component->OnSystemUpdate();
		}

		if (!this->mIsInitComplate)
		{
			return;
		}
		if (this->mStartTimer - mLastUpdateTime >= this->mLogicUpdateInterval)
		{
			this->mLogicRunCount++;
			for (IFrameUpdate *component : this->mFrameUpdateManagers)
			{
				component->OnFrameUpdate(this->mDelatime);
			}

			for (ILastFrameUpdate *component : this->mLastFrameUpdateManager)
			{
				component->OnLastFrameUpdate();
			}
			this->mStartTimer = mLastUpdateTime = TimeHelper::GetMilTimestamp();
		}

		if (this->mStartTimer - this->mSecondTimer >= 1000)
		{
			for (ISecondUpdate *component : this->mSecondUpdateManagers)
			{
				component->OnSecondUpdate();
			}
			this->UpdateConsoleTitle();
			this->mSecondTimer = TimeHelper::GetMilTimestamp();
			if (this->mSecondTimer - this->mNextRefreshTime >= 0)
			{
				this->OnZeroRefresh();
				this->mNextRefreshTime = TimeHelper::GetTomorrowZeroTime() * 1000;
			}
		}
	}

    void App::UpdateConsoleTitle()
    {
        long long nowTime = TimeHelper::GetMilTimestamp();
        float seconds = (nowTime - this->mMainLoopStartTime) / 1000.0f;
        this->mLogicFps = this->mLogicRunCount / seconds;
#ifdef _WIN32
        char buffer[100] = {0};
        sprintf_s(buffer, "%s fps:%f", this->mServerName.c_str(), this->mLogicFps);
        SetConsoleTitle(buffer);
#else
        //SayNoDebugInfo("fps = " << this->mLogicFps);
#endif
    }
}