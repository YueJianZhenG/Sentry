﻿#include "TimerComponent.h"
#include <Timer/DelayTimer.h>
#include"Object/App.h"
namespace Sentry
{
    bool TimerComponent::Awake()
    {
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

    long long TimerComponent::AddTimer(std::shared_ptr<TimerBase> timer)
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
        return this->AddTimer(std::make_shared<DelayTimer>(ms, func));
    }

    bool TimerComponent::CancelTimer(long long id)
    {
        auto iter = this->mTimerMap.find(id);
        if (iter != this->mTimerMap.end())
        {
			std::shared_ptr<TimerBase> timer = iter->second;
			if(timer != nullptr)
			{
				timer->Invoke(TimerState::Cancel);
			}
            this->mTimerMap.erase(iter);
            return true;
        }
        return false;
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
				if(timeWheelLayer->GetLayerId() == 0)
				{
					while(!timerQueue.empty())
					{
						long long id = timerQueue.front();
						auto iter = this->mTimerMap.find(id);
						if(iter != this->mTimerMap.end())
						{
							auto timerBase = iter->second;
							if(timerBase != nullptr)
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
					while(!timerQueue.empty())
					{
						long long id = timerQueue.front();
						this->AddTimerToWheel(id);
						timerQueue.pop();
					}

				}
				if(!timeWheelLayer->JumpNextLayer())
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
		auto timer = iter->second;
		return this->AddTimerToWheel(timer);
	}

    bool TimerComponent::AddTimerToWheel(std::shared_ptr<TimerBase> timer)
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
        LOG_ERROR("add timer failure id = ", timer->GetTimerId());
        return false;
    }
}// namespace Sentry