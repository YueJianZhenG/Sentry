﻿#include "TimerComponent.h"
#include <Timer/DelayTimer.h>
#include"Core/App.h"
namespace GameKeeper
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

	bool TimerComponent::LateAwake()
	{
		this->mNextUpdateTime = TimeHelper::GetMilTimestamp() + this->TimerPrecision;
        return true;
	}

    unsigned int TimerComponent::AddTimer(TimerBase * timer)
    {
        if (timer == nullptr)
        {
            return false;
        }
        timer->mTimerId = this->mTimerIdPool.Pop();
        LOG_CHECK_RET_ZERO(this->AddTimerToWheel(timer));
        return timer->mTimerId;
    }

    unsigned int TimerComponent::AddTimer(long long ms, StaticMethod * func)
    {
        if (ms == 0)
        {
            func->run();
			delete func;
            return 0;
        }
        return this->AddTimer(new DelayTimer(ms, func));
    }

    bool TimerComponent::RemoveTimer(unsigned int id)
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

    TimerBase * TimerComponent::GetTimer(unsigned int id)
    {
        auto iter = this->mTimerMap.find(id);
        return iter != this->mTimerMap.end() ? iter->second : nullptr;
    }

    void TimerComponent::OnSystemUpdate()
    {
		if (this->mNextUpdateTime == 0)
		{
			return;
		}
        long long nowTime = TimeHelper::GetMilTimestamp();
        long long subTime = nowTime - this->mNextUpdateTime;

        if (subTime <= (this->TimerPrecision - 2)) //2毫秒误差
        {
            return;
        }
        int count = subTime / this->TimerPrecision;
        count = count == 0 ? 1 : count;

        this->mNextUpdateTime = nowTime + this->TimerPrecision - subTime;

        for (int index = 0; index < count; index++)
        {
            TimeWheelLayer *timerLayer = this->mTimerLayers[0];

            bool res = timerLayer->MoveIndex(this->mTimers);
            while (!this->mTimers.empty())
            {
                auto id = this->mTimers.front();
                this->mTimers.pop();
                auto iter = this->mTimerMap.find(id);
                if (iter != this->mTimerMap.end())
                {
                    auto timer = iter->second;
                    this->mTimerMap.erase(iter);
                    if(timer->Invoke())
                    {
                        delete timer;
                        continue;
                    }
                    this->AddTimerToWheel(timer);
                }
            }
            for (size_t i = 1; i < this->mTimerLayers.size() && res; i++)
            {
                timerLayer = this->mTimerLayers[i];
                res = timerLayer->MoveIndex(this->mTimers);

                while (!this->mTimers.empty())
                {
                    auto id = this->mTimers.front();
                    this->mTimers.pop();
                    auto timer = this->GetTimer(id);
                    if(timer != nullptr)
                    {
                        this->AddTimerToWheel(timer);
                    }
                }
            }
        }
    }

    bool TimerComponent::InvokeTimer(unsigned int id)
    {
        auto iter = this->mTimerMap.find(id);
        if (iter != this->mTimerMap.end())
        {
            TimerBase * timer = iter->second;
            this->mTimerMap.erase(iter);
            if(timer->Invoke())
            {
                delete timer;
                return true;
            }
            return this->AddTimerToWheel(timer);
        }
        return false;
    }

    bool TimerComponent::AddTimerToWheel(TimerBase * timer)
    {
        long long nowTime = TimeHelper::GetMilTimestamp();
        int tick = (timer->GetTriggerTime() - nowTime) / this->TimerPrecision;
        for (auto timerLayer : this->mTimerLayers)
        {
            if (timerLayer->AddTimer(tick, timer->mTimerId))
            {
                this->mTimerMap.emplace(timer->mTimerId, timer);
                return true;
            }
        }
        delete timer;
        LOG_ERROR("add timer " << timer->GetTimerId() << " failure");
        return false;
    }
}// namespace GameKeeper