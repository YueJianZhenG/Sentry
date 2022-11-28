﻿
#include"App.h"
#include"System/System.h"
#include"Timer/ElapsedTimer.h"
#include"Config/ClusterConfig.h"
#include"File/DirectoryHelper.h"
#include"Service/LuaRpcService.h"
#include"Component/ProtoComponent.h"
#include"Component/LocationComponent.h"
#include"Component/TextConfigComponent.h"
#include"Component/NetThreadComponent.h"
#include"Component/LaunchComponent.h"
using namespace Sentry;
using namespace std::chrono;

namespace Sentry
{

	App::App() : Unit(0),
        mStartTime(Helper::Time::NowMilTime())
	{
        this->mLogicFps = 0;
        this->mTickCount = 0;
        this->mIsStartDone = false;
        this->mLogComponent = nullptr;
        this->mTaskComponent = nullptr;
        this->mTimerComponent = nullptr;
        this->mMessageComponent = nullptr;
        this->mThreadId = std::this_thread::get_id();
	}

	bool App::LoadComponent()
	{
		this->mTaskComponent = this->GetOrAddComponent<TaskComponent>();
		this->mLogComponent = this->GetOrAddComponent<LoggerComponent>();
		this->mTimerComponent = this->GetOrAddComponent<TimerComponent>();
		this->mMessageComponent = this->GetOrAddComponent<ProtoComponent>();

        LOG_CHECK_RET_FALSE(this->AddComponent<TextConfigComponent>());
        LOG_CHECK_RET_FALSE(this->AddComponent<LocationComponent>());
        LOG_CHECK_RET_FALSE(this->AddComponent<NetThreadComponent>());
        LOG_CHECK_RET_FALSE(this->AddComponent<LaunchComponent>());

        std::vector<Component *> components;
        if(this->GetComponents(components) > 0)
        {
            for (Component *component: components)
            {
                if (!this->InitComponent(component))
                {
                    CONSOLE_LOG_ERROR("Init " << component->GetName() << " failure");
                    return false;
                }
            }
        }
        this->mTaskComponent->Start(&App::StartAllComponent, this);
        return true;
	}

	bool App::InitComponent(Component* component)
	{
		if (!component->LateAwake())
		{
			LOG_ERROR(component->GetName() << " late awake ");
			return false;
		}

        RpcService * rpcService = component->Cast<RpcService>();
        if(rpcService != nullptr)
        {
            const std::string & name = component->GetName();
            this->mSeviceMap.emplace(name, rpcService);
        }
		return true;
	}

	int App::Run(int argc, char ** argv)
    {
        if (!System::Init(argc, argv))
        {
            CONSOLE_LOG_FATAL("start failure");
            return -1;
        }

        ServerConfig serverConfig;
        if (!serverConfig.LoadConfig(System::ConfigPath()))
        {
            return false;
        }
        this->mMainThread = std::make_unique<Asio::Context>();
        if (!this->LoadComponent())
        {
            this->GetLogger()->SaveAllLog();
            return -1;
        }
        Asio::ContextWork work(*this->mMainThread);
        const std::chrono::milliseconds sleepTime(1);
        long long logicStartTime = Helper::Time::NowMilTime();
        long long logicSecondTime = Helper::Time::NowMilTime();
        long long logicLastUpdateTime = Helper::Time::NowMilTime();

        std::vector<IFrameUpdate *> frameUpdateComponents;
        std::vector<ISystemUpdate *> systemUpdateComponents;
        std::vector<ISecondUpdate *> secondUpdateComponents;
        std::vector<ILastFrameUpdate *> lastFrameUpdateComponents;
        this->GetComponents<IFrameUpdate>(frameUpdateComponents);
        this->GetComponents<ISystemUpdate>(systemUpdateComponents);
        this->GetComponents<ISecondUpdate>(secondUpdateComponents);
        this->GetComponents<ILastFrameUpdate>(lastFrameUpdateComponents);


        float fps = 15;
        Asio::Code code;
        float deltaTime = 0;
        long long logicRunCount = 0;
        ServerConfig::Inst()->GetMember("fps", fps);
        long long logicUpdateInterval = 1000 / fps;
        while (!this->mMainThread->stopped())
        {
            this->mMainThread->poll(code);
            for (ISystemUpdate *component: systemUpdateComponents)
            {
                component->OnSystemUpdate();
            }
            if (!this->mIsStartDone)
            {
                continue;
            }
            logicStartTime = Helper::Time::NowMilTime();
            if (logicStartTime - logicLastUpdateTime >= logicUpdateInterval)
            {
                logicRunCount++;
                for (IFrameUpdate *component: frameUpdateComponents)
                {
                    component->OnFrameUpdate(deltaTime);
                }

                if (logicStartTime - logicSecondTime >= 1000)
                {
                    this->mTickCount++;
#ifdef __OS_WIN__
                    this->UpdateConsoleTitle();
#endif
                    long long nowTime = Helper::Time::NowMilTime();
                    float seconds = (nowTime - logicSecondTime) / 1000.0f;
                    this->mLogicFps = (float)logicRunCount / seconds;
                    for (ISecondUpdate *component: secondUpdateComponents)
                    {
                        component->OnSecondUpdate(this->mTickCount);
                    }
                    logicRunCount = 0;
                    logicSecondTime = Helper::Time::NowMilTime();
                }

                for (ILastFrameUpdate *component: lastFrameUpdateComponents)
                {
                    component->OnLastFrameUpdate();
                }
                logicLastUpdateTime = Helper::Time::NowMilTime();
            }
            std::this_thread::sleep_for(sleepTime);
        }
        return 0;
    }

	void App::Stop()
    {
        this->mMainThread->stop();
        LOG_WARN("close server successful ");
    }

	void App::StartAllComponent()
    {
        std::vector<IStart*> startComponents;
        this->GetComponents(startComponents);
        for(IStart * component : startComponents)
        {
            ElapsedTimer timer;
            const std::string & name = dynamic_cast<Component*>(component)->GetName();
            long long timeId = this->mTimerComponent->DelayCall(10 * 1000, [name]()
            {
                LOG_FATAL(name << " start time out");
            });
            if(!component->Start())
            {
                LOG_ERROR("start [" << name << "] failure");
                this->Stop();
                return;
            }
            this->mTimerComponent->CancelTimer(timeId);
            LOG_DEBUG("start " << name << " successful use time = [" << timer.GetSecond() << "s]");
        }
        this->mIsStartDone = true; //开始帧循环
        std::vector<IComplete *> completeComponents;
        this->GetComponents(completeComponents);
        for(IComplete * complete : completeComponents)
        {
            complete->OnLocalComplete();
        }
        CONSOLE_LOG_DEBUG("start all component complete");
        this->WaitServerStart();
    }

	void App::WaitServerStart() //等待依赖的服务启动完成
	{
		std::vector<const NodeConfig *> configs;
		ClusterConfig::Inst()->GetNodeConfigs(configs);
		LocationComponent * locationComponent = this->GetComponent<LocationComponent>();
		for(const NodeConfig * nodeConfig : configs)
		{
			int count = 0;
			while(!locationComponent->GetServerCount(nodeConfig->GetName()))
			{
				count++;
				this->mTaskComponent->Sleep(2000);
				LOG_WARN("wait " << nodeConfig->GetName() << " start count = count");
			}
		}
        std::vector<IComplete *> completeComponents;
        this->GetComponents<IComplete>(completeComponents);
        for (IComplete* complete: completeComponents)
        {
            complete->OnClusterComplete();
        }
		long long t = Helper::Time::NowMilTime() - this->mStartTime;
		LOG_INFO("===== start " << ServerConfig::Inst()->Name() << " successful [" << t / 1000.0f << "]s ===========");
	}
#ifdef __OS_WIN__
	void App::UpdateConsoleTitle()
	{
		char buffer[100] = {0};
		const std::string& name = ServerConfig::Inst()->Name();
		sprintf_s(buffer, "%s fps:%f", name.c_str(), this->mLogicFps);
		SetConsoleTitle(buffer);
	}
#endif

	RpcService* App::GetService(const std::string& name)
	{
		auto iter = this->mSeviceMap.find(name);
		return iter != this->mSeviceMap.end() ? iter->second : nullptr;
	}
}