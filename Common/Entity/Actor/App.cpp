#include"App.h"
#include "XCode/XCode.h"
#include "Util/Core/Com.h"
#ifndef __OS_WIN__
#include<csignal>
#endif

#include "Lua/Engine/Define.h"
#include "Core/System/System.h"
#include "Server/Config/ServerConfig.h"
#include "Timer/Timer/ElapsedTimer.h"
#include "Proto/Component/ProtoComponent.h"
#include "Server/Component/ThreadComponent.h"
#include "Cluster/Component/LaunchComponent.h"
#include "Log/Component/LoggerComponent.h"
#include "Timer/Component/TimerComponent.h"
#include "Cluster/Config/ClusterConfig.h"
#include "Config/Base/LangConfig.h"
#include "Event/Base/IEvent.h"
#include "Event/Component/EventProxyComponent.h"
#ifdef __ENABLE_OPEN_SSL__

#include "Auth/Jwt/Jwt.h"

#else

#include "Util/Crypt/Mask.h"
#include "Proto/Bson/base64.h"

#endif

#ifndef __DEBUG__
#include "Http/Component/NotifyComponent.h"
#endif

#ifdef __ENABLE_MI_MALLOC__
#include "mimalloc.h"

inline void* ssl_malloc(size_t size, const char* file, int line) {
	return mi_malloc(size);
}
inline void* ssl_realloc(void* ptr, size_t size, const char* file, int line) {
	return mi_realloc(ptr, size);
}
inline void ssl_free(void* ptr, const char* file, int line) {
	mi_free(ptr);
}

#endif

#ifdef __OS_WIN__
	inline bool HandlerRoutine(DWORD dwCtrlType)
	{
		switch (dwCtrlType)
		{
			case CTRL_C_EVENT:
			case CTRL_CLOSE_EVENT:
			case CTRL_SHUTDOWN_EVENT:
				acs::App::Inst()->Stop();
				return true;
		}
		return false;
	}
#endif

namespace acs
{
	App::App(int id, const std::string & name) :
			Node(id, name), mSignal(mContext),
			mContext(1), mStartTime(help::Time::NowMil())
	{
		this->mFps = 0;
		this->mDeltaTime = 0;
		this->mGuidIndex = 0;
		this->mEventCount = 0;
		this->mLastGuidTime = 0;
		this->mActor = nullptr;
		this->mProto = nullptr;
		this->mStartMemory = 0;
		this->mCoroutine = nullptr;
		this->mStatus = ServerStatus::Init;
#ifdef __DEBUG__
		this->mMainId = std::this_thread::get_id();
#endif

#ifdef __OS_WIN__
		Debug::Init();
#endif
#ifdef __ENABLE_OPEN_SSL__
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		OpenSSL_add_all_algorithms();
		ERR_load_crypto_strings();
#else
#ifdef __ENABLE_MI_MALLOC__
		CRYPTO_set_mem_functions(ssl_malloc, ssl_realloc, ssl_free);
#endif
		OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr);
#endif
#endif

#ifdef __ENABLE_OPEN_WOLF_SSL__

#endif
	}

	bool App::LoadComponent()
	{
		std::string path, cluster;
		if (this->mConfig.GetPath("node", path)) //加载集群配置
		{
			LOG_DEBUG("node path = {}", path);
			TextConfig* config = new ClusterConfig();
			if (!config->LoadConfig(path))
			{
				return false;
			}
		}


		this->AddComponent<ThreadComponent>();
		this->AddComponent<LoggerComponent>();
		LOG_CHECK_RET_FALSE(this->AddComponent<TimerComponent>());
		LOG_CHECK_RET_FALSE(this->AddComponent<LaunchComponent>());
		LOG_CHECK_RET_FALSE(this->AddComponent<CoroutineComponent>());
		LOG_CHECK_RET_FALSE(this->AddComponent<EventProxyComponent>());

		this->mActor = this->GetComponent<NodeComponent>();
		this->mProto = this->GetComponent<ProtoComponent>();
		this->mCoroutine = this->GetComponent<CoroutineComponent>();
		LOG_CHECK_RET_FALSE(this->InitComponent());
#ifndef __OS_WIN__
		this->mSignal.add(SIGINT);
		this->mSignal.add(SIGTERM);
		this->mSignal.async_wait([this](const asio::error_code& code, int signal)
		{
			this->mCoroutine->Start(&App::Stop, this);
		});
#else
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)HandlerRoutine, true);
#endif
		jwt::secret = this->mConfig.GetSecretKey();
		this->mCoroutine->Start(&App::StartAllComponent, this);
		if (this->mActor == nullptr)
		{
			return true;
		}
		LOG_CHECK_RET_FALSE(this->LateAwake())
		return this->mActor->AddCluster(this->mConfig.Name(), this->GetNodeId());
	}

	bool App::LoadLang() const
	{
		std::string path;
		if (!this->mConfig.GetPath("lang", path))
		{
			return true;
		}
		return (new LangConfig())->LoadConfig(path);
	}

	bool App::Refresh()
	{
		std::vector<IRefresh*> hotfixComponents;
		this->GetComponents<IRefresh>(hotfixComponents);
		for (IRefresh* hotfixComponent: hotfixComponents)
		{
			if (!hotfixComponent->OnRefresh())
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
		return this->mCoroutine->Start(std::move(func));
	}

	bool App::InitComponent()
	{
		std::vector<Component*> components;
		this->GetComponents(components);
		for (Component* component: components)
		{
#ifdef __DEBUG__
			//timer::ElapsedTimer timer;
			//LOG_DEBUG("start component => {}", component->GetName())
#endif
			if (!component->LateAwake())
			{
				LOG_ERROR("{} LateAwake fail", component->GetName());
				return false;
			}
#ifdef __DEBUG__
			//LOG_INFO("[{}ms] [{}.LateAwake] ok", timer.GetMs(), component->GetName());
#endif
		}
		return true;
	}

	int App::Run()
	{
		std::string path;
		srand(help::Time::NowMil());
		if(!os::System::GetAppEnv("CONFIG", path))
		{
			return XCode::ConfigError;
		}
		if (!this->mConfig.LoadConfig(path) || !this->LoadLang())
		{
			return XCode::ConfigError;
		}
		if (!this->LoadComponent())
		{
			return XCode::Failure;
		}


		std::vector<IFrameUpdate*> frameUpdateComponents;
		std::vector<ISystemUpdate*> systemUpdateComponents;
		std::vector<ISecondUpdate*> secondUpdateComponents;
		std::vector<ILastFrameUpdate*> lastFrameUpdateComponents;
		this->GetComponents<IFrameUpdate>(frameUpdateComponents);
		this->GetComponents<ISystemUpdate>(systemUpdateComponents);
		this->GetComponents<ISecondUpdate>(secondUpdateComponents);
		this->GetComponents<ILastFrameUpdate>(lastFrameUpdateComponents);


		int fps = 15;
		Asio::Code code;
		int eventCount = 100;
		json::r::Value jsonObject;
		if (this->mConfig.Get("core", jsonObject))
		{
			jsonObject.Get("fps", fps);
			jsonObject.Get("event", eventCount);
		}
		int tickCount = 0;
		int frameCount = 0;
		long long secondTotalTime = 0;
		std::chrono::milliseconds sleepTime(1);
		long long nowFrameTime = help::Time::NowMil();
		long long lastFrameTime = help::Time::NowMil();
		auto work = asio::make_work_guard(this->mContext);
		const long long CONST_UPDATE_INTERVAL = 1000 / fps;
		long long nextFrameTime = help::Time::NowMil() + CONST_UPDATE_INTERVAL;
		while (!this->mContext.stopped())
		{
			int count = 0;
			while (this->mContext.poll_one(code) > 0 && count <= eventCount)
			{
				++count;
				this->mEventCount++;
			}
			nowFrameTime = help::Time::NowMil();
			help::Time::NowTimeMS = nowFrameTime;
			for (ISystemUpdate* component: systemUpdateComponents)
			{
				component->OnSystemUpdate(nowFrameTime);
			}

			if (this->mStatus >= ServerStatus::Ready && this->mStatus
			        < ServerStatus::Closing && nowFrameTime >= nextFrameTime)
			{
				nextFrameTime = nowFrameTime + CONST_UPDATE_INTERVAL;
				this->mDeltaTime = (int)(nowFrameTime - lastFrameTime);
				for (IFrameUpdate* component: frameUpdateComponents)
				{
					component->OnFrameUpdate(this->mDeltaTime);
				}
				++frameCount;
				secondTotalTime += this->mDeltaTime;
				if (secondTotalTime >= 1000)
				{
					++tickCount;
					this->mFps = (float)frameCount * 1000.0f / (float)secondTotalTime;
#ifdef __OS_WIN__
					os::SystemInfo systemInfo;
					os::System::GetSystemInfo(systemInfo);
					SetConsoleTitle(fmt::format("fps:{:.2f} [{:.2f}%] {}/{}",
							this->mFps, systemInfo.cpu * 100, help::com::BytesToString(systemInfo.use_memory),
							help::com::BytesToString(systemInfo.max_memory)).c_str());
#endif
#if defined(__OS_WIN__) || defined(__OS_MAC__)
					this->Refresh();
#endif
					help::OnOneSecondEvent::Trigger(tickCount);
					for (ISecondUpdate* component: secondUpdateComponents)
					{
						component->OnSecondUpdate(tickCount);
					}
					frameCount = 0;
					secondTotalTime = 0;
				}
				for (ILastFrameUpdate* component: lastFrameUpdateComponents)
				{
					component->OnLastFrameUpdate();
				}
				lastFrameTime = nowFrameTime;
			}
			std::this_thread::sleep_for(sleepTime);
		}
#ifdef __OS_WIN__
		Debug::Clear();
#endif
		printf("========== close server ==========\n");
		return XCode::Ok;
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
		int nodeId = this->GetNodeId();
		return nowTime << 31 | (int)nodeId << 16 | index;
	}

	std::string App::NewUuid()
	{
		return std::to_string(this->MakeGuid());
	}

	void App::Stop()
	{
		if (this->mStatus == ServerStatus::Closing)
		{
			return;
		}
		this->mStatus = ServerStatus::Closing;
#ifdef __DEBUG__
		CONSOLE_LOG_ERROR("start close {}", this->Name());
#else
		long long t1 = help::Time::NowMil();
#endif
		std::vector<IAppStop*> stopComponent;
		this->GetComponents<IAppStop>(stopComponent);

		for (IAppStop* component: stopComponent)
		{
			component->OnAppStop(); //保存数据
		}

		std::vector<IDestroy*> components;
		this->GetComponents<IDestroy>(components);
		std::reverse(components.begin(), components.end());
		for (size_t index = 0; index < components.size(); index++)
		{
			IDestroy* nextComponent = nullptr;
			IDestroy* component = components.at(index);
			if (index < components.size() - 1)
			{
				nextComponent = components.at(index + 1);
			}
			std::string name2("null");
			float process = (index + 1) / (float)components.size();
			const std::string& name1 = dynamic_cast<Component*>(component)->GetName();
			if (nextComponent != nullptr)
			{
				name2 = dynamic_cast<Component*>(nextComponent)->GetName();
			}
			component->OnDestroy();
		}
		this->mContext.stop();
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
			const long long timeId = timerComponent->Timeout(10 * 1000, [name]()
			{
				LOG_FATAL("{} start time out", name);
			});
			component->OnStart();
			timerComponent->CancelTimer(timeId);
			if (timer.GetMs() > 100)
			{
				LOG_DEBUG("[{}ms] => {}.Start Done", timer.GetMs(), name);
			}
		}
		startComponents.clear();
		this->mStatus = ServerStatus::Start; //开始帧循环
		std::vector<IComplete*> completeComponents;
		this->GetComponents<IComplete>(completeComponents);
		for (IComplete* complete: completeComponents)
		{
			timer::ElapsedTimer timer;
			Component* component = dynamic_cast<Component*>(complete);
			const std::string& name = dynamic_cast<Component*>(component)->GetName();
			const long long timerId = timerComponent->Timeout(1000 * 10, [component]()
			{
				LOG_ERROR("{0}.Complete call timeout", component->GetName());
			});
			complete->OnComplete();
			if (timer.GetMs() > 100)
			{
				LOG_DEBUG("[{}ms] => {}.Complete", timer.GetMs(), name);
			}
			timerComponent->CancelTimer(timerId);
		}
		completeComponents.clear();
		this->mStatus = ServerStatus::Ready;
#ifdef __DEBUG__
		os::SystemInfo systemInfo;
		os::System::GetSystemInfo(systemInfo);
		this->mStartMemory = systemInfo.use_memory;
#endif
		long long t = help::Time::NowMil() - this->mStartTime;
		LOG_BY_NAME("app", "  ===== start {} ok [{:.3f}s] =======", this->Name(), t / 1000.0f);

		//VLDEnable();
	}

	int App::Run(const std::string& cmd)
	{
		return XCode::Ok;
	}
}