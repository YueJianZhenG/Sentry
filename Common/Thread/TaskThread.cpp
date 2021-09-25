﻿#include "TaskThread.h"
#include <Scene/TaskComponent.h>
#include <functional>

using namespace std::chrono;
namespace Sentry
{
    TaskThread::TaskThread(TaskComponent *manager, int index)
        : mBindThread(std::bind(&TaskThread::Run, this))
    {
        this->mIsStop = false;
        this->mTaskState = Idle;
        this->mTaskManager = manager;
        //this->mBindThread = new std::thread(std::bind(&TaskThread::Run, this));
    }

    void TaskThread::AddTask(TaskProxy * task)
    {
        this->mTaskState = ThreadState::Run;
        this->mWaitInvokeTask.Add(task);
        this->mThreadVariable.notify_one();
    }

    void TaskThread::Run()
    {
		this->mThreadId = std::this_thread::get_id();

        while (!this->mIsStop)
        {
			TaskProxy * task = nullptr;
            this->mWaitInvokeTask.SwapQueueData();
            while (this->mWaitInvokeTask.PopItem(task))
            {
				task->Run();
				this->mFinishTasks.push(task->GetTaskId());       
            }

			while (!this->mFinishTasks.empty())
			{
				unsigned int id = this->mFinishTasks.front();
				this->mFinishTasks.pop();
				this->mTaskManager->PushFinishTask(id);
			}

            this->mTaskState = ThreadState::Idle;
            std::unique_lock<std::mutex> waitLock(this->mThreadLock);
            this->mThreadVariable.wait(waitLock);
        }
    }
}// namespace Sentry
