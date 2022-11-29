//
// Created by yjz on 2022/1/5.
//
#include"WaitTaskSourceBase.h"

namespace Sentry
{
    WaitTaskSourceBase::WaitTaskSourceBase()
    {
        this->mCorId = 0;
        this->mState = TaskState::TaskReady;
        this->mTaskId = Helper::Guid::Create();
        this->mCreateTime = Helper::Time::NowMilTime();
        this->mTaskComponent = App::Inst()->GetTaskComponent();
    }

	void WaitTaskSourceBase::Clear()
	{
		this->mCorId = 0;
		this->mState = TaskState::TaskReady;
		this->mTaskId = Helper::Guid::Create();
		this->mCreateTime = Helper::Time::NowMilTime();
	}

    bool WaitTaskSourceBase::ResumeTask(TaskState state)
    {
        switch(this->mState)
		{
		case TaskState::TaskReady:
			this->mState = state;
			return true;
		case TaskState::TaskAwait:
			this->mState = state;
			this->mTaskComponent->Resume(this->mCorId);
			return true;
		case TaskState::TaskFinish:
			break;
		}
        return false;
    }

    bool WaitTaskSourceBase::YieldTask()
    {
        if(this->mState == TaskState::TaskReady)
        {
            this->mState = TaskState::TaskAwait;
			assert(this->mTaskComponent->YieldCoroutine(this->mCorId));
            return true;
        }
        return false;
    }
}

