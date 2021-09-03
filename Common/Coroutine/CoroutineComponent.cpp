﻿#include"CoroutineComponent.h"
#include<chrono>
#include<memory.h>
#include<Core/App.h>
#include"Coroutine.h"
#include<Util/NumberHelper.h>
#include<Timer/TimerComponent.h>
#include<Timer/CorSleepTimer.h>
#include<NetWork/NetWorkRetAction.h>


using namespace std::chrono;
#ifdef _WIN32
#include<Windows.h>
#endif
namespace Sentry
{
#ifdef __COROUTINE_ASM__
	void MainEntry(tb_context_from_t parame)
	{
		CoroutineComponent *pCoroutineMgr = (CoroutineComponent*)parame.priv;
		Coroutine * mainCoroutine = pCoroutineMgr->GetCoroutine(0);
		mainCoroutine->mCorContext = parame.ctx;
#else
#if _WIN32
	void __stdcall MainEntry(LPVOID manager)
	{
		CoroutineComponent *pCoroutineMgr = (CoroutineComponent *)manager;
#else
	void MainEntry(void *manager)
	{
		CoroutineComponent *pCoroutineMgr = (CoroutineComponent *)manager;
#endif
#endif
		Coroutine * logicCoroutine = pCoroutineMgr->GetCoroutine();
		//SayNoDebugError("Invoke Coroutine Id " << logicCoroutine->mCoroutineId);
		if (logicCoroutine != nullptr)
		{
			logicCoroutine->mFunction->run();
			pCoroutineMgr->Destory(logicCoroutine);
#ifdef __COROUTINE_ASM__
			tb_context_jump(parame.ctx, nullptr);
#endif
		}
	}

    CoroutineComponent::CoroutineComponent()
		:mCorPool(100)
    {
		this->mMainCoroutine = this->mCorPool.Pop();
#ifdef __COROUTINE_ASM__		
		
#elif _WIN32		
		this->mMainCoroutine->mContextStack = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
#endif
		this->mCurrentCorId = 0;
    }

    bool CoroutineComponent::Awake()
    {
		SayNoAssertRetFalse_F(this->mTimerManager = Scene::GetComponent<TimerComponent>());
        return true;
    }

    void CoroutineComponent::Start()
    {
		/*this->StartCoroutine(&CoroutineComponent::Loop, this);
		this->StartCoroutine(&CoroutineComponent::Loop2, this);*/
    }

    void CoroutineComponent::Sleep(long long ms)
    {
        Coroutine *logicCoroutine = this->GetCoroutine();
        if (logicCoroutine != nullptr)
        {
            const long long id = logicCoroutine->mCoroutineId;
            this->mTimerManager->CreateTimer<CorSleepTimer>(this, id, ms);
            this->YieldReturn();
        }
    }

	void CoroutineComponent::ResumeCoroutine()
	{
		Coroutine * logicCoroutine = this->GetCoroutine();
		if (logicCoroutine == nullptr)
		{
			return;
		}
#ifdef __COROUTINE_ASM__
		tb_context_from_t from;
#endif
		if (logicCoroutine->mState == CorState::Ready)
		{
#ifdef __COROUTINE_ASM__
			if (logicCoroutine->mStack == nullptr)
			{
				logicCoroutine->mStackSize = 1024 * 2;
				logicCoroutine->mStack = (char *)malloc(1024 * 2);
				logicCoroutine->mStackTop = logicCoroutine->mStack + logicCoroutine->mStackSize;
			}
			//this->SaveStack(logicCoroutine, nullptr);
			logicCoroutine->mCorContext = tb_context_make(
				logicCoroutine->mStack, logicCoroutine->mStackSize, MainEntry);
			SayNoDebugLog("resume new coroutine id " << logicCoroutine->mCoroutineId);
			from = tb_context_jump(logicCoroutine->mCorContext, this);
#elif _WIN32
			logicCoroutine->mContextStack = CreateFiber(STACK_SIZE, MainEntry, this);
			SwitchToFiber(logicCoroutine->mContextStack);
#elif __linux__
			getcontext(&logicCoroutine->mCorContext);
			logicCoroutine->mState = CorState::Running;
			logicCoroutine->mCorContext.uc_stack.ss_size = STACK_SIZE;
			logicCoroutine->mCorContext.uc_stack.ss_sp = this->mSharedStack;
			logicCoroutine->mCorContext.uc_link = &this->mMainCoroutine->mCorContext;
			makecontext(&logicCoroutine->mCorContext, (void(*)(void)) MainEntry, 1, this);
			swapcontext(&this->mMainCoroutine->mCorContext, &logicCoroutine->mCorContext);
#endif
		}
		else if (logicCoroutine->mState == CorState::Suspend)
		{			
			logicCoroutine->mState = CorState::Running;
#ifdef __COROUTINE_ASM__			
			SayNoDebugLog("from " << mMainCoroutine->mCoroutineId << " swap to " << logicCoroutine->mCoroutineId);
			from = tb_context_jump(logicCoroutine->mCorContext, this);
#elif _WIN32
			SwitchToFiber(logicCoroutine->mContextStack);
#elif __linux__
			void *start = this->mSharedStack + STACK_SIZE - logicCoroutine->mStackSize;
			memcpy(start, logicCoroutine->mContextStack, logicCoroutine->mStackSize);
			swapcontext(&this->mMainCoroutine->mCorContext, &logicCoroutine->mCorContext);
#endif

#ifdef __COROUTINE_ASM__
			logicCoroutine->mCorContext = from.ctx;
#endif
		}
	}

	void CoroutineComponent::Resume(unsigned int id)
	{
		if (id == 0)
		{
			return;
		}
		Coroutine *logicCoroutine = this->GetCoroutine(id);
		if (logicCoroutine == nullptr)
		{
			SayNoDebugFatal("not find coroutine id : " << id);
			return;
		}
		this->mResumeCors.push(id);		
	}

	unsigned int CoroutineComponent::StartCoroutine(MethodProxy * func)
	{
		Coroutine * coroutine = this->mCorPool.Pop();
		if (coroutine != nullptr)
		{
#ifndef __COROUTINE_ASM__
	#ifdef _WIN32
			if (coroutine->mContextStack != nullptr)
			{
				DeleteFiber(coroutine->mContextStack);
				coroutine->mStackSize = 0;
				coroutine->mContextStack = nullptr;
			}
	#endif
#endif
			coroutine->mFunction = func;
			coroutine->mState = CorState::Ready;
			this->Resume(coroutine->mCoroutineId);
			return coroutine->mCoroutineId;
		}
		return 0;
	}

	void CoroutineComponent::Loop()
	{
		for (int index = 0; index < 1000; index++)
		{
			SayNoDebugWarning(__FUNCTION__ << " " << __LINE__ << " index = " << index);
			this->Sleep(1000);
		}
	}

	void CoroutineComponent::Loop2()
	{
		for (int index = 0; index < 1000; index++)
		{
			SayNoDebugError(__FUNCTION__ << " " << __LINE__ << " index = " << index);
			this->Sleep(1000);
		}
	}

	void CoroutineComponent::YieldReturn()
    {
        Coroutine *logicCoroutine = this->GetCoroutine();
        if (logicCoroutine == nullptr)
        {
            SayNoDebugFatal("not find coroutine context");
            return;
        }
		if (logicCoroutine->mCoroutineId == 0)
		{
			SayNoDebugFatal("try yield main coroutine");
			return;
		}
        this->mCurrentCorId = 0;
        logicCoroutine->mState = CorState::Suspend;
#ifdef __COROUTINE_ASM__
		tb_context_from_t from;
		from = tb_context_jump(this->mMainCoroutine->mCorContext, nullptr);
		this->mMainCoroutine->mCorContext = from.ctx;
#elif _WIN32
        SwitchToFiber(this->mMainCoroutine->mContextStack);
#else
        this->SaveStack(logicCoroutine, this->mSharedStack + STACK_SIZE);
        swapcontext(&logicCoroutine->mCorContext, &this->mMainCoroutine->mCorContext);
#endif
    }

    Coroutine *CoroutineComponent::GetCoroutine()
    {
		if (this->mCurrentCorId == 0)
		{
			return nullptr;
		}
		return this->mCorPool.Get(this->mCurrentCorId);     
    }

    Coroutine *CoroutineComponent::GetCoroutine(unsigned int id)
    {
		return this->mCorPool.Get(id);
    }

	void CoroutineComponent::Destory(Coroutine * coroutine)
	{
		this->mCurrentCorId = 0;
		this->mCorPool.Push(coroutine);
#ifdef __COROUTINE_ASM__
		
#elif _WIN32
		SwitchToFiber(this->mMainCoroutine->mContextStack);
#else
		setcontext(&this->mMainCoroutine->mCorContext);
#endif
	}

    long long CoroutineComponent::GetNowTime()
    {
        auto timeNow = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        return timeNow.count();
    }

    void CoroutineComponent::SaveStack(Coroutine *cor, char *top)
    {
#ifdef __COROUTINE_ASM__
		size_t size = cor->mStackTop - (char *)cor->mCorContext;
		memcpy(cor->mStack, cor->mCorContext, size);
#else
		char dummy = 0;
		size_t size = top - &dummy;
		if (cor->mStackSize < size)
		{
			free(cor->mContextStack);
			cor->mContextStack = malloc(size);
		}
		cor->mStackSize = size;
		memcpy(cor->mContextStack, &dummy, size);
#endif
          
    }

    void CoroutineComponent::OnSystemUpdate()
    {
		while (!this->mResumeCors.empty())
		{
			this->mCurrentCorId = this->mResumeCors.front();
			this->ResumeCoroutine();
			this->mResumeCors.pop();
		}
		this->mCurrentCorId = 0;
    }
}