#include "TimerBase.h"
namespace Sentry
{
    TimerBase::TimerBase(long long ms)
    {
        this->mStartTime = Helper::Time::GetMilTimestamp();
        this->mTimerId = Helper::Guid::Create();
        this->mTargetTime = this->mStartTime + ms;
    }
}// namespace Sentry