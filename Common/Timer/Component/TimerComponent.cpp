#include"TimerComponent.h"
#include"Timer/Timer/DelayTimer.h"
#include"Lua/Engine/ModuleClass.h"
#include "Lua/Lib/Lib.h"
#include "Entity/Actor/App.h"
#include "Util/Tools/TimeHelper.h"
namespace config
{
	constexpr int LayerCount = 3;  //时间轮的层数
	constexpr int TimerPrecision = 50; //定时器精度 100就是0.1s, 50就是0.05s
	constexpr int OtherLayerCount = 60;
	constexpr int FirstLayerCount = 60 * 1000 / TimerPrecision;
}

namespace acs
{
	TimerComponent::TimerComponent()
	{
		this->mIndex = 0;
		this->mNowTime = 0;
		this->mDoneCount = 0;
		this->mCancelCount = 0;
		this->mNextUpdateTime = 0;
		this->mTimerMap.reserve(100);
		this->mTimerMap.max_load_factor(0.75);
	}

	bool TimerComponent::Awake()
	{
		LuaCCModuleRegister::Add([](Lua::CCModule & ccModule) {
			ccModule.Open("core.timer", lua::lib::luaopen_ltimer);
		});

		for (int index = 0; index < config::LayerCount; index++)
		{
			int count = index == 0
						? config::FirstLayerCount : config::OtherLayerCount;
			unsigned int end = config::FirstLayerCount * (unsigned int)std::pow(config::OtherLayerCount, index);

			unsigned int start = index == 0 ? 0 :
						config::FirstLayerCount * (unsigned int)std::pow(config::OtherLayerCount, index - 1);
			this->mTimerLayers.emplace_back(index + 1, count, start, end);
		}
		return true;
	}

	void TimerComponent::OnStart()
	{
		long long nowTime = help::Time::NowSec();
		long long diffDayTime = help::Time::GetNextDayTime() - nowTime;
		this->mNextUpdateTime = nowTime * 1000 + config::TimerPrecision;
		long long diffHourTime = help::Time::GetNextHourTime() - nowTime;
		long long diffMinuteTime = help::Time::GetNextMinuteTime() - nowTime;
		this->Timeout((int)(diffHourTime * 1000), &TimerComponent::OnHour, this);
		this->Timeout((int)(diffDayTime * 1000), &TimerComponent::OnNewDay, this);
		this->Timeout((int)(diffMinuteTime * 1000), &TimerComponent::OnMinute, this);
	}

	void TimerComponent::OnDestroy()
	{
		this->mTimerMap.clear();
	}


	long long TimerComponent::MakeTimerID()
	{
		return help::ID::Gen();
	}

	long long TimerComponent::MakeTimerID(int type, int id)
	{
		long long result = type;
		return result << 32 | id;
	}

	void TimerComponent::OnHour()
	{
		long long nowTime = help::Time::NowSec();
		help::Time::Date timeDate = help::Time::GetTimeDate(nowTime);
		{
			help::OnNewHourEvent::Trigger(timeDate.day, timeDate.hour);
			long long diffHourTime = help::Time::GetNextHourTime() - nowTime;
			this->Timeout((int)(diffHourTime * 1000), &TimerComponent::OnHour, this);
		}
	}

	void TimerComponent::OnNewDay()
	{
		long long nowTime = help::Time::NowSec();
		help::Time::Date timeDate = help::Time::GetTimeDate(nowTime);
		{
			help::OnNewDayEvent::Trigger(timeDate.day, timeDate.week);
			long long diffDayTime = help::Time::GetNextDayTime() - nowTime;
			this->Timeout((int)(diffDayTime * 1000), &TimerComponent::OnNewDay, this);
		}
	}

	void TimerComponent::OnMinute()
	{
		long long nowTime = help::Time::NowSec();
		help::Time::Date timeDate = help::Time::GetTimeDate(nowTime);
		{
			//LOG_DEBUG("hour:{} minute:{}", timeDate.hour, timeDate.minute)
			help::OnNewMinuteEvent::Trigger(timeDate.hour, timeDate.minute);
			int diffMinuteTime = (int)(help::Time::GetNextMinuteTime() - nowTime);
			this->Timeout(diffMinuteTime * 1000, &TimerComponent::OnMinute, this);
		}
	}


	bool TimerComponent::AddTimer(std::unique_ptr<TimerBase> timer)
	{
		if(timer->GetTargetTime() == 0)
		{
			timer->Refresh(this->mNowTime);
		}
		return this->AddTimerToWheel(std::move(timer));
	}

	long long TimerComponent::CreateTimer(int ms, std::unique_ptr<StaticMethod> func)
	{
		long long id = TimerComponent::MakeTimerID();
		return this->CreateTimer(id, ms, std::move(func));
	}

	long long TimerComponent::CreateTimer(long long id, int ms, std::unique_ptr<StaticMethod> func)
	{
		std::unique_ptr<TimerBase> timerBase = std::make_unique<DelayTimer>(id, ms, std::move(func));
		if (timerBase->GetTimeoutMS() <= config::TimerPrecision)
		{
			this->mLastFrameTriggerTimers.emplace(timerBase->mTimerId);
			this->mTimerMap.emplace(timerBase->mTimerId, std::move(timerBase));
			return 0;
		}
		timerBase->Refresh(this->mNowTime);
		return this->AddTimer(std::move(timerBase)) ? id : 0;
	}

	bool TimerComponent::CancelTimer(long long id)
	{
		auto iter = this->mTimerMap.find(id);
		if (iter == this->mTimerMap.end())
		{
			return false;
		}
		this->mCancelCount++;
		this->mDelTimers.emplace(id);
		return true;
	}

	void TimerComponent::OnSystemUpdate(long long nowTime)
	{
		this->mNowTime = nowTime;
		while(this->mNextUpdateTime != 0 && nowTime >= this->mNextUpdateTime)
		{
			this->OnTick();
			this->mNextUpdateTime += config::TimerPrecision;
		}
	}

	void TimerComponent::OnTick()
	{
		while(!this->mDelTimers.empty())
		{
			long long id = this->mDelTimers.front();
			auto iter = this->mTimerMap.find(id);
			if(iter != this->mTimerMap.end())
			{
				this->mTimerMap.erase(iter);
			}
			this->mDelTimers.pop();
		}
		size_t soltIndex = 0;
		TimeWheelLayer & timeWheelLayer = this->mTimerLayers.front();

		std::queue<long long>& timerQueue = timeWheelLayer.GetTimerQueue(soltIndex);
		while (!timerQueue.empty())
		{
			long long id = timerQueue.front();
			this->InvokeTimer(id);
			timerQueue.pop();
		}
		if(soltIndex < timeWheelLayer.GetSoltCount())
		{
			return;
		}
		for(size_t index = 1; index < this->mTimerLayers.size(); index++)
		{
			TimeWheelLayer & moveWheelLayer = this->mTimerLayers.at(index);
			{
				std::queue<long long>& moveTimerQueue = moveWheelLayer.GetTimerQueue(soltIndex);
				{
					while(!moveTimerQueue.empty())
					{
						this->AddTimerToWheel(moveTimerQueue.front());
						moveTimerQueue.pop();
					}
				}
				CONSOLE_LOG_INFO("[{}] solt index => {}", moveWheelLayer.GetLayerId(), soltIndex)
				if(soltIndex < moveWheelLayer.GetSoltCount())
				{
					break;
				}
			}
		}
	}

	void TimerComponent::OnRecord(json::w::Document& document)
	{
		std::unique_ptr<json::w::Value> jsonValue = document.AddObject("timer");
		{
			jsonValue->Add("wait", this->mTimerMap.size());
			jsonValue->Add("done", this->mDoneCount);
			jsonValue->Add("cancel", this->mCancelCount);
		}
	}

	void TimerComponent::OnLastFrameUpdate()
	{
		if(!this->mLastFrameTriggerTimers.empty())
		{
			std::swap(this->mTriggerTimers, this->mLastFrameTriggerTimers);
			while(!this->mTriggerTimers.empty())
			{
				this->InvokeTimer(this->mTriggerTimers.front());
				this->mTriggerTimers.pop();
			}
		}
	}

	bool TimerComponent::InvokeTimer(long long timerId)
	{
		auto iter = this->mTimerMap.find(timerId);
		if (iter == this->mTimerMap.end())
		{
			return false;
		}
		this->mDoneCount++;
		try
		{
			iter->second->Invoke();
		}
		catch(const std::exception & e)
		{
			LOG_FATAL("timer_id:{} =>{}", timerId, e.what());
		}
		catch(...)
		{
			LOG_FATAL("timer_id:{} => unknown error", timerId);
		}
		this->mTimerMap.erase(iter);
		return true;
	}

	bool TimerComponent::AddTimerToWheel(long long timerId)
	{
		auto iter = this->mTimerMap.find(timerId);
		if (iter == this->mTimerMap.end())
		{
			return false;
		}
		long long nowTime = help::Time::NowMil();
		std::unique_ptr<TimerBase> & timer = iter->second;
		long long subTime = timer->GetTargetTime() - nowTime;
		if (subTime <= config::TimerPrecision)
		{
			this->mLastFrameTriggerTimers.push(timer->mTimerId);
			return true;
		}
		size_t index = 0;
		unsigned int tick = subTime / config::TimerPrecision;
		for (TimeWheelLayer & timerLayer : this->mTimerLayers)
		{
			if (timerLayer.AddTimer(tick, timer->mTimerId, index))
			{
				return true;
			}
		}
		long long targetTime = timer->GetTargetTime() / 1000;
		const std::string time = help::Time::GetDateString(targetTime);
		LOG_ERROR("add timer failure id:{} target_time:{}", timer->GetTimerId(), time);
		return false;
	}

	bool TimerComponent::AddTimerToWheel(std::unique_ptr<TimerBase> timer)
	{
		long long useTime = timer->GetTargetTime() - this->mNowTime;
		if (useTime <= config::TimerPrecision)
		{
			this->mLastFrameTriggerTimers.push(timer->mTimerId);
			this->mTimerMap.emplace(timer->mTimerId, std::move(timer));
			return true;
		}
		size_t index = 0;
		unsigned int tick = useTime / config::TimerPrecision;
		for (TimeWheelLayer & timerLayer : this->mTimerLayers)
		{
			if (timerLayer.AddTimer(tick, timer->mTimerId, index))
			{
// #ifdef __DEBUG__
// 				long long target = timer->GetTargetTime() / 1000;
// 				std::string time = help::Time::GetDateString(target);
// 				LOG_INFO("add timer layer:{} index:{} target:{}", timerLayer.GetLayerId(), index, time)
// #endif
				this->mTimerMap.emplace(timer->mTimerId, std::move(timer));
				return true;
			}
		}
		long long targetTime = timer->GetTargetTime() / 1000;
		const std::string time = help::Time::GetDateString(targetTime);
		LOG_ERROR("add timer failure id {} target_time:{}", timer->GetTimerId(), time);
		return false;
	}

	long long TimerComponent::Timeout(int ms, std::function<void()>&& callback)
	{
		return this->CreateTimer(ms, std::make_unique<LambdaMethod>(std::move(callback)));
	}
}// namespace Sentry