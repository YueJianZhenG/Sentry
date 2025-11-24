
#pragma once
#include"queue"
#include"TimerBase.h"

namespace acs
{
    class TimeWheelLayer
    {
    public:
        TimeWheelLayer(int layerId, int count, unsigned int min, unsigned int max);
    public:
		std::queue<long long> & GetTimerQueue(size_t & index);
		inline int GetLayerId() const { return this->mLayerId;}
		inline size_t GetLayerIndex() const { return this->mCurIndex;}
    	bool AddTimer(unsigned int tick, long long timerId);
    	bool AddTimer(unsigned int tick, long long timerId, size_t & index);
    	inline size_t GetSoltCount() const { return this->mTimerSlot.size(); }
    private:
		const int mLayerId;
		const int mMaxCount;
        const unsigned int mMin;
        const unsigned int mMax;
    private:
        size_t mCurIndex;
        std::vector<std::queue<long long>> mTimerSlot;
    };
}