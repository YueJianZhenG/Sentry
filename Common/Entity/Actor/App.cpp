﻿#include"App.h"

#ifndef __OS_WIN__

#include<csignal>

#endif

#include "Lua/Engine/Define.h"
#include "Core/System/System.h"
#include "Server/Config/ServerConfig.h"
#include "Timer/Timer/ElapsedTimer.h"
//#include "Util/File/DirectoryHelper.h"
#include "Proto/Component/ProtoComponent.h"
#include "Server/Component/ConfigComponent.h"
#include "Server/Component/ThreadComponent.h"
#include "Cluster/Component/LaunchComponent.h"
#include "Log/Component/LoggerComponent.h"
#include "Timer/Component/TimerComponent.h"
#include "Http/Component/GroupNotifyComponent.h"

#include "Config/Base/LangConfig.h"

#ifdef __ENABLE_OPEN_SSL__

#include "Auth/Aes/Aes.h"
#include "Auth/Jwt/Jwt.h"

#endif
namespace joke
{
	App::App(int id, ServerConfig& config) :
			Server(id, config.Name()), mSignal(mContext),
			mContext(1), mThreadId(std::this_thread::get_id()),
			mStartTime(help::Time::NowMil()), mConfig(config)
	{
		this->mLogicFps = 0;
		this->mTickCount = 0;
		this->mGuidIndex = 0;
		this->mLastGuidTime = 0;
		this->mTaskComponent = nullptr;
		this->mActorComponent = nullptr;
		this->mMessageComponent = nullptr;
		this->mStatus = ServerStatus::Init;
#ifdef __ENABLE_OPEN_SSL__
		aes::Init();
#endif
	}

	bool App::LoadComponent()
	{

		this->AddComponent<ThreadComponent>();
		this->AddComponent<LoggerComponent>();
		LOG_CHECK_RET_FALSE(this->AddComponent<TimerComponent>());
		LOG_CHECK_RET_FALSE(this->AddComponent<ProtoComponent>());
		LOG_CHECK_RET_FALSE(this->AddComponent<ActorComponent>());
		LOG_CHECK_RET_FALSE(this->AddComponent<ConfigComponent>());
		LOG_CHECK_RET_FALSE(this->AddComponent<LaunchComponent>());
		LOG_CHECK_RET_FALSE(this->AddComponent<CoroutineComponent>());

		this->mActorComponent = this->GetComponent<ActorComponent>();
		this->mMessageComponent = this->GetComponent<ProtoComponent>();
		this->mTaskComponent = this->GetComponent<CoroutineComponent>();

		LOG_CHECK_RET_FALSE(this->InitComponent());
#ifndef __OS_WIN__
		this->mSignal.add(SIGINT);
		this->mSignal.add(SIGTERM);
		this->mSignal.async_wait([this](const asio::error_code& code, int signal)
		{
			this->mTaskComponent->Start(&App::Stop, this);
		});
#endif
		this->mTaskComponent->Start(&App::StartAllComponent, this);
		return this->mActorComponent->AddServer(this);
	}

	bool App::LoadLang()
	{
		std::string path;
		if (!this->mConfig.GetPath("lang", path))
		{
			return false;
		}
		return (new LangConfig())->LoadConfig(path);
	}

	bool App::Hotfix()
	{
		std::vector<IHotfix*> hotfixComponents;
		this->GetComponents<IHotfix>(hotfixComponents);
		for (IHotfix* hotfixComponent: hotfixComponents)
		{
			if (!hotfixComponent->OnHotFix())
			{
				Component* component = dynamic_cast<Component*>(hotfixComponent);
				LOG_ERROR("{} invoke hotfix failure", component->GetName());
				return false;
			}
		}
		return true;
	}

	unsigned int App::StartCoroutine(std::function<void()>&& func)
	{
		return this->mTaskComponent->Start(std::move(func));
	}

	bool App::InitComponent()
	{
		std::vector<Component*> components;
		this->GetComponents(components);
		for (Component* component: components)
		{
#ifdef __DEBUG__
			timer::ElapsedTimer timer;
			//LOG_DEBUG("start component => {}", component->GetName())
#endif
			if (!component->LateAwake())
			{
				LOG_ERROR("{} LateAwake fail", component->GetName());
				return false;
			}
#ifdef __DEBUG__
			LOG_DEBUG("[{}ms] [{}.LateAwake] ok", timer.GetMs(), component->GetName());
#endif
		}
		return true;
	}

	void App::Sleep(int ms)
	{
		this->mTaskComponent->Sleep(ms);
	}

	int App::Run()
	{
		srand(help::Time::NowMil());
		if(!this->LoadLang())
		{
			return XServerCode::ConfError;
		}
		if (!this->LoadComponent())
		{
#ifdef __OS_WIN__
			return getchar();
#else
			return XServerCode::InitError;
#endif
		}

		long long logicStartTime = 0;
		long long logicSecondTime = help::Time::NowMil();
		long long logicLastUpdateTime = help::Time::NowMil();

		std::vector<IFrameUpdate*> frameUpdateComponents;
		std::vector<ISystemUpdate*> systemUpdateComponents;
		std::vector<ISecondUpdate*> secondUpdateComponents;
		std::vector<ILastFrameUpdate*> lastFrameUpdateComponents;
		std::vector<ICoroutineSecond*> coroutineSecondComponents;

		this->GetComponents<IFrameUpdate>(frameUpdateComponents);
		this->GetComponents<ISystemUpdate>(systemUpdateComponents);
		this->GetComponents<ISecondUpdate>(secondUpdateComponents);
		this->GetComponents<ICoroutineSecond>(coroutineSecondComponents);
		this->GetComponents<ILastFrameUpdate>(lastFrameUpdateComponents);


		int fps = 15;
		Asio::Code code;
		int event = 100;
		long long logicRunCount = 0;
		Asio::ContextWork work(this->mContext);
		std::unique_ptr<json::r::Value> jsonObject;
		if (this->mConfig.Get("core", jsonObject))
		{
			jsonObject->Get("fps", fps);
			jsonObject->Get("event", event);
		}
		std::chrono::milliseconds sleepTime(1);
		long long logicUpdateInterval = 1000 / fps;

		while (!this->mContext.stopped())
		{
			logicRunCount++;
			this->mContext.poll(code);
			for (ISystemUpdate* component: systemUpdateComponents)
			{
				component->OnSystemUpdate();
			}
			if (this->mStatus >= ServerStatus::Start && this->mStatus < ServerStatus::Closing)
			{
				logicStartTime = help::Time::NowMil();
				if (logicStartTime - logicLastUpdateTime >= logicUpdateInterval)
				{
					for (IFrameUpdate* component: frameUpdateComponents)
					{
						component->OnFrameUpdate();
					}
					long long nowTime = help::Time::NowMil();
					logicUpdateInterval = (1000 / fps) - (nowTime - logicStartTime);

					if (logicStartTime - logicSecondTime >= 1000)
					{
#ifdef __OS_MAC__
						this->Hotfix();
#elif __OS_WIN__
						this->Hotfix();
#endif
						this->mTickCount++;
						long long costTime = nowTime - logicSecondTime;
						float seconds = (float)costTime / 1000.0f;
						this->mLogicFps = (float)logicRunCount / seconds;
						for (ISecondUpdate* component: secondUpdateComponents)
						{
							component->OnSecondUpdate(this->mTickCount);
						}
						for (ICoroutineSecond* component: coroutineSecondComponents)
						{
							this->mTaskComponent->Start(&ICoroutineSecond::OnCoroutineSecond, component,
									this->mTickCount);
						}

						logicRunCount = 0;
						logicSecondTime = help::Time::NowMil();
					}

					for (ILastFrameUpdate* component: lastFrameUpdateComponents)
					{
						component->OnLastFrameUpdate();
					}
					logicLastUpdateTime = help::Time::NowMil();
				}
			}
#ifndef __OS_WIN__
			std::this_thread::sleep_for(sleepTime);
#endif
		}
#ifdef __OS_WIN__
		return std::getchar();
#else
		printf("========== close server ==========\n");
		return XServerCode::Ok;
#endif
	}

	long long App::MakeGuid()
	{
		long long nowTime = help::Time::NowSec();
		if (nowTime != this->mLastGuidTime)
		{
			this->mGuidIndex = 0;
			this->mLastGuidTime = nowTime;
		}
		int index = this->mGuidIndex++;
		if (index >= std::numeric_limits<unsigned short>::max())
		{
			nowTime++;
			index = 0;
			this->mGuidIndex = 0;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		int serverId = this->GetSrvId();
		return nowTime << 31 | (int)serverId << 16 | index;
	}

	std::string App::NewUuid()
	{
		long long guid = this->MakeGuid();
		return std::to_string(guid);
	}

#ifdef __ENABLE_OPEN_SSL__

	std::string App::Sign(json::w::Document& document)
	{
		std::string data;
		document.Encode(&data);
		return jwt::Create(data, this->mConfig.GetSecretKey());
	}

	bool App::DecodeSign(const std::string& sign, json::r::Document& document)
	{
		std::string data;
		if (!jwt::Verify(sign, this->mConfig.GetSecretKey(), data))
		{
			return false;
		}
		return document.Decode(data);
	}

#endif

	void App::Stop()
	{
		if (this->mStatus == ServerStatus::Closing)
		{
			return;
		}
		this->mStatus = ServerStatus::Closing;
#ifdef __DEBUG__
		CONSOLE_LOG_ERROR("start close {}", this->Name());
#endif
		long long t1 = help::Time::NowMil();

		std::vector<IAppStop *> stopComponent;
		this->GetComponents<IAppStop>(stopComponent);

		for(size_t index = 0; index < stopComponent.size(); index++)
		{
			IAppStop * component = stopComponent.at(index);
			if(component != nullptr)
			{
				component->OnAppStop(); //保存数据
			}
		}

		std::vector<IDestroy*> components;
		this->GetComponents<IDestroy>(components);
		std::reverse(components.begin(), components.end());
		for(size_t index = 0; index < components.size(); index++)
		{
			IDestroy* nextComponent = nullptr;
			IDestroy* component = components.at(index);
			if(index < components.size() -1)
			{
				nextComponent = components.at(index + 1);
			}
			component->OnDestroy();
			std::string name2("null");
			float process = (index + 1) / (float)components.size();
			const std::string & name1 = dynamic_cast<Component*>(component)->GetName();
			if(nextComponent != nullptr)
			{
				name2 = dynamic_cast<Component*>(nextComponent)->GetName();
			}
			CONSOLE_LOG_INFO("[{:.2f}%] close {} => {}", process * 100, name1, name2);
		}
#ifndef __DEBUG__
		long long t = help::Time::NowMil() - t1;
		GroupNotifyComponent* notify = this->GetComponent<GroupNotifyComponent>();
		{
			notify::TemplateCard cardInfo;
			cardInfo.Jump.url = "https://huwai.pro";
			cardInfo.title = LangConfig::Text("server_stop_notify");
			cardInfo.data.emplace_back(LangConfig::Text("cost_time"), fmt::format("{:.2f}s", t / 1000.f));
			cardInfo.data.emplace_back(LangConfig::Text("process"), fmt::format("{}:{}", this->mConfig.Name(), this->GetSrvId()));
			cardInfo.data.emplace_back(LangConfig::Text("time"), help::Time::GetDateString());

			notify->SendToWeChat(cardInfo, true);
		}
#endif
		this->mContext.stop();
		this->GetComponent<ThreadComponent>()->CloseThread();
	}

	void App::StartAllComponent()
	{
		std::vector<IStart*> startComponents;
		this->GetComponents<IStart>(startComponents);
		TimerComponent* timerComponent = this->GetComponent<TimerComponent>();
		for (IStart* component: startComponents)
		{
			timer::ElapsedTimer timer;
			const std::string& name = dynamic_cast<Component*>(component)->GetName();
			long long timeId = timerComponent->DelayCall(10 * 1000, [name]()
			{
				LOG_FATAL("{} start time out", name);
			});
			component->Start();
			timerComponent->CancelTimer(timeId);
			if (timer.GetMs() > 0)
			{
				LOG_DEBUG("[{}ms] => {}.Start", timer.GetMs(), name);
			}
		}
		this->mStatus = ServerStatus::Start; //开始帧循环
		std::vector<IComplete*> completeComponents;
		this->GetComponents<IComplete>(completeComponents);
		for (IComplete* complete: completeComponents)
		{
			Component* component = dynamic_cast<Component*>(complete);
			const long long timerId = timerComponent->DelayCall(1000 * 10, [component]()
			{
				LOG_ERROR("{0}.Complete call timeout", component->GetName());
			});
			complete->Complete();
			timerComponent->CancelTimer(timerId);
		}
		this->mStatus = ServerStatus::Ready;
		long long t = help::Time::NowMil() - this->mStartTime;
#ifndef __DEBUG__
		GroupNotifyComponent* notify = this->GetComponent<GroupNotifyComponent>();
		if(notify != nullptr)
		{
			notify::TemplateCard cardInfo;
			cardInfo.Jump.url = "https://huwai.pro";
			cardInfo.title = LangConfig::Text("server_start_notify");
			cardInfo.data.emplace_back(LangConfig::Text("cost_time"), fmt::format("{:.2f}s", t / 1000.f));
			cardInfo.data.emplace_back(LangConfig::Text("process"), fmt::format("{}:{}", this->mConfig.Name(), this->GetSrvId()));
			cardInfo.data.emplace_back(LangConfig::Text("config"), this->mConfig.Path());
			cardInfo.data.emplace_back(LangConfig::Text("time"), help::Time::GetDateString());

			notify->SendToWeChat(cardInfo);
		}
#endif
		LOG_INFO("  ===== start {} ok [{:.2f}s] =======", this->Name(), t / 1000.0f);
	}
}