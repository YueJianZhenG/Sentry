﻿#include"TimerComponent.h"
#include"Timer/Timer/DelayTimer.h"
#include"Entity/Actor/App.h"
#include"Lua/Engine/ClassProxyHelper.h"
#include"Timer/Lua/Timer.h"
namespace Tendo
{
	bool TimerComponent::Awake()
	{
		this->mNextUpdateTime = 0;
		for (int index = 0; index < this->LayerCount; index++)
		{
			int count = index == 0
						? this->FirstLayerCount : this->OtherLayerCount;
			int end = FirstLayerCount * (int)std::pow(OtherLayerCount, index);

			int start = index == 0 ? 0 :
						FirstLayerCount * (int)std::pow(OtherLayerCount, index - 1);
			this->mTimerLayers.push_back(new TimeWheelLayer(index, count, start, end));
		}
        return true;
	}

	long long TimerComponent::AddTimer(const std::shared_ptr<TimerBase>& timer)
	{
		if (timer == nullptr || !this->AddTimerToWheel(timer))
		{
			return 0;
		}
		return timer->GetTimerId();
	}

	long long TimerComponent::AddTimer(unsigned int ms, StaticMethod* func)
	{
		if (ms == 0)
		{
			func->run();
			delete func;
			return 0;
		}
		return this->AddTimer(std::make_shared<DelayTimer>(ms, func));
	}

    void TimerComponent::OnLuaRegister(Lua::ClassProxyHelper &luaRegister)
    {
        luaRegister.BeginNewTable("Timer");
        luaRegister.PushExtensionFunction("Add", Lua::Timer::Add);
        luaRegister.PushExtensionFunction("Remove", Lua::Timer::Remove);
    }

	bool TimerComponent::CancelTimer(long long id)
	{
		auto iter = this->mTimerMap.find(id);
		if (iter != this->mTimerMap.end())
		{
            this->mRemoveTimers.push(id);
			return true;
		}
		return false;
	}

	void TimerComponent::OnSystemUpdate()
	{
		if (this->mNextUpdateTime == 0)
		{
			this->mNextUpdateTime = Helper::Time::NowMilTime();
			return;
		}
		long long nowTime = Helper::Time::NowMilTime();
		long long subTime = nowTime - this->mNextUpdateTime;
		const int tick = (int)subTime / this->TimerPrecision;

		if (tick <= 0) return;

        while(!this->mRemoveTimers.empty())
        {
            long long id = this->mRemoveTimers.front();
            auto iter = this->mTimerMap.find(id);
            if(iter != this->mTimerMap.end())
            {
                this->mTimerMap.erase(iter);
            }
            this->mRemoveTimers.pop();
        }

		this->mNextUpdateTime = nowTime - (subTime % this->TimerPrecision);

		for (int count = 0; count < tick; count++)
		{
			for (auto & timeWheelLayer : this->mTimerLayers)
			{
				std::queue<long long>& timerQueue = timeWheelLayer->GetTimerQueue();
				if (timeWheelLayer->GetLayerId() == 0)
				{
					while (!timerQueue.empty())
					{
						long long id = timerQueue.front();
						auto iter = this->mTimerMap.find(id);
						if (iter != this->mTimerMap.end())
						{
							auto timerBase = iter->second;
							if (timerBase != nullptr)
							{
								timerBase->Invoke();
							}
							this->mTimerMap.erase(iter);
						}
						timerQueue.pop();
					}
				}
				else
				{
					while (!timerQueue.empty())
					{
						long long id = timerQueue.front();
						this->AddTimerToWheel(id);
						timerQueue.pop();
					}

				}
				if (!timeWheelLayer->JumpNextLayer())
				{
					break;
				}
			}
		}
	}

	bool TimerComponent::AddTimerToWheel(long long timerId)
	{
		auto iter = this->mTimerMap.find(timerId);
		if (iter == this->mTimerMap.end())
		{
			return false;
		}
		auto timer = iter->second;
		return this->AddTimerToWheel(timer);
	}

	bool TimerComponent::AddTimerToWheel(std::shared_ptr<TimerBase> timer)
	{
		long long nowTime = Helper::Time::NowMilTime();
		int tick = (timer->GetTargetTime() - nowTime) / this->TimerPrecision;
		if (tick <= 0)
		{
			timer->Invoke();
			return true;
		}
		for (auto timerLayer : this->mTimerLayers)
		{
			if (timerLayer->AddTimer(tick, timer->mTimerId))
			{
				this->mTimerMap.emplace(timer->mTimerId, timer);
				return true;
			}
		}
		LOG_ERROR("add timer failure id = " << timer->GetTimerId());
		return false;
	}

	long long TimerComponent::DelayCall(int ms, std::function<void()>&& callback)
	{
		LambdaMethod * lambdaMethod = new LambdaMethod(std::move(callback));
		return this->AddTimer(ms, lambdaMethod);
	}
}// namespace Sentry