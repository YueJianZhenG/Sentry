﻿#include "TimerComponent.h"
#include <Timer/DelayTimer.h>
#include"Object/App.h"
namespace Sentry
{
    bool TimerComponent::Awake()
    {
		this->mLayerIndex = 0;
		this->mNextUpdateTime = 0;
        for (int index = 0; index < this->LayerCount; index++)
        {
            int count = index == 0
                    ? this->FirstLayerCount : this->OtherLayerCount;
            int end = FirstLayerCount * std::pow(OtherLayerCount, index);

            int start = index == 0 ? 0 :
                    FirstLayerCount * std::pow(OtherLayerCount, index - 1);
            this->mTimerLayers.push_back(new TimeWheelLayer(index, count, start, end));
        }
        return true;
    }

    long long TimerComponent::AddTimer(TimerBase * timer)
    {
		if(this->mNextUpdateTime == 0)
		{
			this->mNextUpdateTime = Helper::Time::GetMilTimestamp();
		}
        if (timer == nullptr || !this->AddTimerToWheel(timer))
        {
            return 0;
        }
        return timer->GetTimerId();
    }

    long long TimerComponent::AddTimer(unsigned int ms, StaticMethod * func)
    {
        if (ms == 0)
        {
            func->run();
			delete func;
            return 0;
        }
        return this->AddTimer(new DelayTimer(ms, func));
    }

    bool TimerComponent::RemoveTimer(long long id)
    {
        auto iter = this->mTimerMap.find(id);
        if (iter != this->mTimerMap.end())
        {
            delete iter->second;
            this->mTimerMap.erase(iter);
            return true;
        }
        return false;
    }

    TimerBase * TimerComponent::GetTimer(long long id)
    {
        auto iter = this->mTimerMap.find(id);
        return iter != this->mTimerMap.end() ? iter->second : nullptr;
    }

    void TimerComponent::OnSystemUpdate()
	{
		long long nowTime = Helper::Time::GetMilTimestamp();
		long long subTime = nowTime - this->mNextUpdateTime;
		const int tick = subTime / this->TimerPrecision;

		if (tick <= 0 || this->mNextUpdateTime == 0)
		{
			return;
		}

		this->mNextUpdateTime = nowTime - (subTime % this->TimerPrecision);

		for(int count = 0; count < tick; count++)
		{
			for (size_t index = 0; index < this->mTimerLayers.size(); index++)
			{
				TimeWheelLayer* timeWheelLayer = this->mTimerLayers[index];
				std::queue<long long>& timerQueue = timeWheelLayer->GetTimerQueue();
				while(!timerQueue.empty())
				{
					long long id = timerQueue.front();
					this->AddTimerToWheel(id);
					timerQueue.pop();
				}
				if(!timeWheelLayer->MoveIndex())
				{
					break;
				}
			}
		}
	}

	bool TimerComponent::AddTimerToWheel(long long timerId)
	{
		auto iter = this->mTimerMap.find(timerId);
		if(iter == this->mTimerMap.end())
		{
			return false;
		}
		TimerBase * timer = iter->second;
		if(this->AddTimerToWheel(timer))
		{
			delete timer;
			this->mTimerMap.erase(iter);
			return true;
		}
		return false;
	}

    bool TimerComponent::AddTimerToWheel(TimerBase * timer)
    {
        long long nowTime = Helper::Time::GetMilTimestamp();
        int tick = (timer->GetTargetTime() - nowTime) / this->TimerPrecision;
        if(tick <= 0)
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
        delete timer;
        LOG_ERROR("add timer failure id = ", timer->GetTimerId());
        return false;
    }
}// namespace Sentry