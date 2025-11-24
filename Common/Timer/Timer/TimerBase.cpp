#include "TimerBase.h"
#include"Util/Tools/TimeHelper.h"
namespace acs
{
    TimerBase::TimerBase(long long id, int ms) : mInterval(ms)
    {
		this->mTimerId = id;
    	this->mTargetTime = 0;
    }
}