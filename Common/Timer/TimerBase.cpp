#include "TimerBase.h"
namespace Sentry
{
    TimerBase::TimerBase(long long ms)
    {
		this->mTimerId = Helper::Guid::Create();
        this->mStartTime = Helper::Time::GetMilTimestamp();
        this->mTargetTime = this->mStartTime + ms;
    }
}// namespace Sentry