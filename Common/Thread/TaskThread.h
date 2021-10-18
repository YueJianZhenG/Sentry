﻿#pragma once

#include "TaskProxy.h"
#include <Other/DoubleBufferQueue.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <Define/CommonDef.h>
#include <Other/DoubleBufferQueue.h>
namespace Sentry
{
	class StaticMethod;
    class TaskPoolComponent;
    enum ThreadState
    {
        Idle,
        Run,
    };

    class IThread
    {
    public:
        IThread(TaskPoolComponent *taskComponent);

        virtual ~IThread()
        {}

    public:
        void Stop()
        { this->mIsClose = true; }

        virtual void AddTask(TaskProxy *task) = 0;

        const std::thread::id GetThreadId()
        { return this->mThreadId; }

    protected:
        virtual void Update() { };

    private:
        void Run();

    private:
        bool mIsClose;
        std::thread *mThread;
        std::thread::id mThreadId;
    protected:
        TaskPoolComponent *mTaskComponent;
    };

    class TaskThread : public IThread
    {
    public:
        TaskThread(TaskPoolComponent * taskComponent);

    public:
        void AddTask(TaskProxy * task) final;
        ThreadState GetTaskState() { return this->mTaskState; }
        bool IsRunning() { return this->mTaskState == ThreadState::Run; }

    protected:
        void Update() final;
    private:
        std::mutex mThreadLock;
        ThreadState mTaskState;
		std::queue<unsigned int> mFinishTasks;
        std::condition_variable mThreadVariable;
        DoubleBufferQueue<TaskProxy *> mWaitInvokeTask;
    };

    class NetWorkThread : public IThread
    {
    public:
        NetWorkThread(TaskPoolComponent * taskComponent, class StaticMethod * method = nullptr);
    public:
        void AddTask(TaskProxy * task) final;
		void AddTask(StaticMethod * task);
		AsioContext & GetContext() { return *mAsioContext; }
    protected:
        void Update() final;
    private:
		AsioWork * mAsioWork;
		AsioContext * mAsioContext;
        StaticMethod * mMethodProxy;
        std::queue<unsigned int> mFinishTasks;
        DoubleBufferQueue<TaskProxy *> mWaitInvokeTask;
		DoubleBufferQueue<StaticMethod *> mWaitInvokeMethod;
    };

	class MainTaskScheduler
	{
	public:
		MainTaskScheduler(StaticMethod * method);
	public:
		int Run();
		void Stop();
		void AddMainTask(StaticMethod * task);		
	private:
		bool mIsStop;
		StaticMethod * mMainMethod;
		DoubleBufferQueue<StaticMethod *> mTaskQueue;
	};
}// namespace Sentry