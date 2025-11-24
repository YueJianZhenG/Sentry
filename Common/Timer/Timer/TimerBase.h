#pragma once

#include"Util/Tools/Guid.h"


namespace acs
{
    class TimerBase
    {
    public:
        friend class TimerComponent;
        explicit TimerBase(long long id, int ms);

        virtual ~TimerBase() = default;

    public:
        virtual void Invoke() = 0;
    public:
		inline long long GetTimerId() const { return mTimerId; }
    	inline int GetTimeoutMS() const { return this->mInterval; }
        inline long long GetTargetTime() const { return this->mTargetTime; }
    	inline void Refresh(long long nowTime) { this->mTargetTime = nowTime + this->mInterval; }
	protected:
		int mInterval;
		long long mTimerId;
		long long mTargetTime;
	};
}// namespace Sentry