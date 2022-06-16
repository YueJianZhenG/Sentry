﻿#include"TaskComponent.h"
#include"App/App.h"
#include"Util/Guid.h"
#include"Coroutine/TaskContext.h"
#include"Component/Timer/TimerComponent.h"
using namespace std::chrono;
namespace Sentry
{
	void MainEntry(tb_context_from_t context)
	{
		TaskComponent * taskComponent = (TaskComponent*)context.priv;
		if (taskComponent != nullptr)
		{
			taskComponent->RunTask(context.ctx);
		}
	}

	void TaskComponent::RunTask(tb_context_t context)
	{
		this->mMainContext = context;
		if (this->mRunContext != nullptr)
		{
			this->mRunContext->Invoke();
			int sid = this->mRunContext->sid;
			Stack& stack = this->mSharedStack[sid];
			if (stack.co == this->mRunContext->mCoroutineId)
			{
				stack.co = 0;
			}
			this->mCorPool.Push(this->mRunContext);
		}
		tb_context_jump(this->mMainContext, nullptr);
	}

	void TaskComponent::Awake()
	{
		this->mRunContext = nullptr;
		for (Stack& stack : this->mSharedStack)
		{
			stack.co = 0;
			stack.size = STACK_SIZE;
			stack.p = new char[STACK_SIZE];
			stack.top = (char*)stack.p + STACK_SIZE;
		}
	}

	bool TaskComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mTimerComponent = this->GetComponent<TimerComponent>());
		return true;
	}

	void TaskComponent::WhenAny(TaskContext* coroutine)
	{
		if (this->mRunContext == nullptr)
		{
			LOG_FATAL("please in coroutine wait");
			return;
		}
		coroutine->mGroup = new CoroutineGroup(1);
		this->YieldCoroutine();
	}

	void TaskComponent::WhenAll(std::vector<TaskContext*>& coroutines)
	{
		if (this->mRunContext == nullptr)
		{
			LOG_FATAL("please in coroutine wait");
			return;
		}
		auto group = new CoroutineGroup(coroutines.size());
		for (auto coroutine : coroutines)
		{
			coroutine->mGroup = group;
		}
		this->YieldCoroutine();
	}

	void TaskComponent::Sleep(long long ms)
	{
		unsigned int id = this->mRunContext->mCoroutineId;
		StaticMethod* sleepMethod = NewMethodProxy(
			&TaskComponent::Resume, this, id);
		this->mTimerComponent->AddTimer(ms, sleepMethod);
		this->YieldCoroutine();
	}

	void TaskComponent::ResumeContext(TaskContext* co)
	{
		co->mState = CorState::Running;
		Stack& stack = mSharedStack[co->sid];
		if (co->mContext == nullptr)
		{
			if (stack.co != co->mCoroutineId)
			{
				this->SaveStack(stack.co);
				stack.co = co->mCoroutineId;
			}
			this->mRunContext->mContext = tb_context_make(stack.p, stack.size, MainEntry);
		}
		else if (stack.co != co->mCoroutineId)
		{
			this->SaveStack(stack.co);
			stack.co = co->mCoroutineId;
			memcpy(co->mContext, co->mStack.p, co->mStack.size);
		}
		tb_context_from_t from = tb_context_jump(co->mContext, this);
		if (from.priv != nullptr)
		{
			this->mRunContext->mContext = from.ctx;
		}
	}

	bool TaskComponent::YieldCoroutine()
	{
		assert(this->mRunContext);
		assert(this->mRunContext->mState == CorState::Running);

		this->mRunContext->mSwitchCount++;
		this->mRunContext->mState = CorState::Suspend;
		tb_context_jump(this->mMainContext, this->mRunContext);
		return true;
	}

	void TaskComponent::Resume(unsigned int id)
	{
		if (id != 0)
		{
			this->mResumeContexts.Push(id);
			return;
		}
		std::cerr << "try resume context id : " << id << std::endl;
	}

	TaskContext* TaskComponent::MakeContext(StaticMethod* func)
	{
		TaskContext* coroutine = this->mCorPool.Pop();
		if (coroutine != nullptr)
		{
			coroutine->mFunction = func;
			coroutine->mState = CorState::Ready;
		}
		return coroutine;
	}

	bool TaskComponent::YieldCoroutine(unsigned int& mCorId)
	{
		if (this->mRunContext != nullptr)
		{
			mCorId = this->mRunContext->mCoroutineId;
			return this->YieldCoroutine();
		}
		LOG_FATAL("not coroutine context");
		return false;
	}

	void TaskComponent::SaveStack(unsigned int id)
	{
		if (id == 0) return;
		TaskContext* coroutine = this->mCorPool.Get(id);
		if (coroutine == nullptr)
		{
			return;
		}
		char* top = this->mSharedStack[coroutine->sid].top;
		size_t size = top - (char*)coroutine->mContext;
		if (coroutine->mStack.size < size)
		{
			coroutine->mStack.p = (char*)realloc(coroutine->mStack.p, size);
			assert(coroutine->mStack.p);
		}
		coroutine->mStack.size = size;
		memcpy(coroutine->mStack.p, coroutine->mContext, size);
	}

	void TaskComponent::OnSystemUpdate()
	{
		unsigned int contextId = 0;
		this->mResumeContexts.Swap();
		while (this->mResumeContexts.Pop(contextId))
		{
			TaskContext* logicCoroutine = this->mCorPool.Get(contextId);
			if(logicCoroutine == nullptr)
			{
				LOG_FATAL("not find task context : " << contextId);
				continue;
			}
			if (logicCoroutine->mState == CorState::Ready
				|| logicCoroutine->mState == CorState::Suspend)
			{
				this->mRunContext = logicCoroutine;
				this->ResumeContext(logicCoroutine);
			}
			this->mRunContext = nullptr;
		}
	}
	void TaskComponent::OnLastFrameUpdate()
	{
		while (!this->mLastQueues.empty())
		{
			unsigned int id = this->mLastQueues.front();
			TaskContext* coroutine = this->mCorPool.Get(id);
			if (coroutine != nullptr)
			{
				this->mResumeContexts.Push(id);
			}
			this->mLastQueues.pop();
		}
	}
}
