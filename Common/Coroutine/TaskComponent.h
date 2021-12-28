﻿#pragma once

#include<list>
#include<queue>
#include<stack>
#include<memory>
#include<tuple>
#include<functional>
#include"Coroutine.h"
#include<Component/Component.h>
namespace GameKeeper
{
    class TaskComponent : public Component,
            public ISystemUpdate, public ILastFrameUpdate, public ISecondUpdate
	{
	public:
		TaskComponent() = default;
		~TaskComponent() final = default;
	public:
		template<typename F, typename T, typename ... Args>
		Coroutine * Start(F && f, T * o, Args &&... args) {
            Coroutine *co = this->CreateCoroutine(
                    NewMethodProxy(std::forward<F>(f),
                                   o, std::forward<Args>(args)...));
            this->Resume(co->mCoroutineId);
            return co;
        }
        Coroutine * Start(std::function<void()> func){
            Coroutine * co = this->CreateCoroutine(new LambdaMethod(func));
            this->Resume(co->mCoroutineId);
            return co;
        }
        Coroutine * CreateCoroutine(StaticMethod * func);
	public:
		void Await();

		void Await(unsigned int & mCorId);

		void AwaitSleep(long long ms);

		void Resume(unsigned int id);

	protected:
		bool Awake() final;

		bool LateAwake() final;

		void OnSystemUpdate() final;

		void OnLastFrameUpdate() final;

        void OnSecondUpdate() final;

		int GetPriority() override { return 2; }

	public:

		void Destory(Coroutine * coroutine);

		Coroutine *GetCoroutine(unsigned int id);

        Coroutine * GetCurCoroutine();

        unsigned int GetCurrentCorId() const
        {
            if(this->mRunCoroutine == nullptr)
            {
                return 0;
            }
            return this->mRunCoroutine->mCoroutineId;
        }

        void RunTask(tb_context_t context);

		bool IsInMainCoroutine() const
		{
			return this->mRunCoroutine == nullptr;
		}

	private:
        void Test(int index);
        void SaveStack(unsigned int id);
        void ResumeCoroutine(Coroutine * co);
	private:
		class TimerComponent *mTimerManager;
		std::queue<unsigned int> mLastQueues;
	private:
		CoroutinePool mCorPool;
        tb_context_t mMainContext;
        Coroutine * mRunCoroutine;
		Stack mSharedStack[SHARED_STACK_NUM];
		std::queue<Coroutine *> mResumeCoroutines;
	};
}