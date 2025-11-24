#include"TimeWheelLayer.h"
namespace acs
{
    TimeWheelLayer::TimeWheelLayer(int layerId, int count, unsigned int min,  unsigned int max)
        : mMin(min), mMax(max), mLayerId(layerId), mMaxCount(count)
    {
        this->mCurIndex = 0;
        for (int index = 0; index < count; index++)
        {
            std::queue<long long> timers;
            this->mTimerSlot.push_back(timers);
        }
    }

    bool TimeWheelLayer::AddTimer(unsigned int tick, long long timerId, size_t & index)
	{
		if (tick >= this->mMin && tick < this->mMax)
		{
			index = this->mMin == 0 ? tick : (tick - this->mMin) / this->mMin;
			if (index + this->mCurIndex < this->mMaxCount)
			{
				index += this->mCurIndex;
				this->mTimerSlot[index].push(timerId);
			}
			else
			{
				index = index + this->mCurIndex - this->mMaxCount;
				this->mTimerSlot[index].push(timerId);
			}
			return true;
		}
		return false;
	}

	bool TimeWheelLayer::AddTimer(unsigned int tick, long long timerId)
	{
		size_t index = 0;
    	return this->AddTimer(tick, timerId, index);
	}


	std::queue<long long> & TimeWheelLayer::GetTimerQueue(size_t & index)
	{
    	index = this->mCurIndex + 1;
		if(index >= this->mTimerSlot.size())
		{
			this->mCurIndex = 0;
		}
		return this->mTimerSlot.at(this->mCurIndex++);
	}
}// namespace Sentry