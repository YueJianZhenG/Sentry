#include"CoroutineComponent.h"
#include"Entity/Actor/App.h"
#include "Util/Core/Com.h"
#include"Timer/Component/TimerComponent.h"
#ifdef __ENABLE_MI_MALLOC__
#include "mimalloc.h"
#endif

namespace acs
{
	inline void MainEntry(tb_context_from_t context)
	{
		CoroutineComponent * taskComponent = (CoroutineComponent*)context.priv;
		if (taskComponent != nullptr)
		{
			taskComponent->RunTask(context.ctx);
		}
	}

	CoroutineComponent::CoroutineComponent()
	{
		this->mIndex = 0;
		this->mCount = 0;
		this->mTimer = nullptr;
#ifdef __ENABLE_SHARE_STACK__
		this->mConfig.pool = 100;
		this->mConfig.share = 8;
		this->mConfig.stack = 1024 * 1024;
#else
		this->mConfig.pool = 100;
		this->mConfig.stack = 128;
#endif
		this->mRunContext = nullptr;
		this->mMainContext = nullptr;
		this->mCoroutines.max_load_factor(0.75);
		REGISTER_JSON_CLASS_FIELD(coroutine::Config, pool);
#ifdef __ENABLE_SHARE_STACK__
		REGISTER_JSON_CLASS_FIELD(coroutine::Config, share);
#endif
		REGISTER_JSON_CLASS_FIELD(coroutine::Config, stack);
	}
#ifdef __ENABLE_SHARE_STACK__
	CoroutineComponent::~CoroutineComponent() noexcept
	{
		for(int index = 0; index < this->mConfig.share; index++)
		{
			Stack& stack = this->mSharedStack[index];
			{
				stack.co = 0;
				stack.size = this->mConfig.stack;
#ifdef __ENABLE_MI_MALLOC__
				mi_free(stack.p);
#else
				free(stack.p);
#endif
			}
		}
	}
#endif

	void CoroutineComponent::RunTask(tb_context_t context)
	{
		this->mMainContext = context;
		if (this->mRunContext != nullptr)
		{
			this->mRunContext->Invoke();
		}
		tb_context_jump(this->mMainContext, nullptr);
	}

	bool CoroutineComponent::Awake()
	{
		this->mRunContext = nullptr;
		if(ServerConfig::Inst()->Get("coroutine", this->mConfig))
		{
			this->mConfig.stack = this->mConfig.share * 1024;
		}
#ifdef __ENABLE_SHARE_STACK__
		this->mSharedStack = std::make_unique<Stack[]>(this->mConfig.share);
		for(int index = 0; index < this->mConfig.share; index++)
		{
			Stack& stack = this->mSharedStack[index];
			{
				stack.co = 0;
				stack.size = this->mConfig.stack;
#ifdef __ENABLE_MI_MALLOC__
				stack.p = (char *)mi_malloc(this->mConfig.stack);
#else
				stack.p = (char *)malloc(this->mConfig.stack);
#endif
				stack.top = stack.p + this->mConfig.stack;
			}
			std::memset(stack.p, 0, this->mConfig.stack);
		}
#else

#endif
		this->mCoroutines.reserve(this->mConfig.pool * 2);
		LOG_DEBUG("coroutine stack:{} pool:{}", help::com::BytesToString(this->mConfig.stack), this->mConfig.pool)
		return true;
	}

	bool CoroutineComponent::LateAwake()
	{
		this->mTimer = this->GetComponent<TimerComponent>();
		return true;
	}

	bool CoroutineComponent::OnRefresh()
	{
		while(!this->mObjectPool.empty())
		{
			this->mObjectPool.pop();
		}
		return true;
	}

	size_t CoroutineComponent::GetMemory()
	{
		size_t size = 0;
		std::queue<std::unique_ptr<TaskContext>> tempQueue;
		while(!this->mObjectPool.empty())
		{
			std::unique_ptr<TaskContext> & coroutine = this->mObjectPool.front();
			{
				size += coroutine->stack.size;
				tempQueue.emplace(std::move(coroutine));
			}
			this->mObjectPool.pop();
		}
		std::swap(this->mObjectPool, tempQueue);
		for(const auto & mCoroutine : this->mCoroutines)
		{
			size += mCoroutine.second->stack.size;
		}
		return size;
	}

	void CoroutineComponent::OnRecord(json::w::Document& document)
	{
		size_t memory = this->GetMemory();
		std::unique_ptr<json::w::Value> data = document.AddObject("coroutine");
		{
#ifdef __ENABLE_SHARE_STACK__
			for(int index = 0; index < this->mConfig.share; index++)
			{
				memory += this->mSharedStack[index].size;
			}
			data->Add("share", this->mConfig.share);
#endif
			data->Add("sum", this->mCount);
			data->Add("wait", this->GetWaitCount());
			data->Add("pool", this->mObjectPool.size());
			data->Add("count", this->mCoroutines.size());
			data->Add("memory", help::com::BytesToString(memory));
		}
	}

	bool CoroutineComponent::Sleep(unsigned int ms)
	{
		if(this->mRunContext == nullptr)
		{
			return false;
		}
		unsigned int id = this->mRunContext->id;
		this->mTimer->Timeout(ms, &CoroutineComponent::Resume, this, id);
		return this->YieldCoroutine();
	}

	bool CoroutineComponent::WaitNextFrame()
	{
		if(this->mRunContext == nullptr)
		{
			return false;
		}
		this->mLastQueues.emplace(this->mRunContext->id);
		this->YieldCoroutine();
		return true;
	}

	bool CoroutineComponent::SetTimeout(int timeout)
	{
		if(this->mRunContext == nullptr)
		{
			return false;
		}
		if(this->mRunContext->timerId > 0)
		{
			this->mTimer->CancelTimer(this->mRunContext->timerId);
		}
		unsigned int id = this->mRunContext->id;
		this->mRunContext->timerId = this->mTimer->Timeout(timeout, &CoroutineComponent::Resume, this, id);;
		return true;
	}

	void CoroutineComponent::RunCoroutine(TaskContext * coroutine)
	{
		this->mRunContext = coroutine;
		unsigned int id = coroutine->id;
//		std::cout << "sid:" << coroutine->sid <<
//			"[" << (int)coroutine->status << "]" "run coroutine " << id << std::endl;
		this->mRunContext->status = CorState::Running;
#ifdef __ENABLE_SHARE_STACK__
		Stack& stack = mSharedStack[this->mRunContext->sid];
		if (this->mRunContext->ctx == nullptr)
		{
			if (stack.co != this->mRunContext->id)
			{
				this->SaveStack(stack.co);
				stack.co = this->mRunContext->id;
			}
			this->mRunContext->ctx = tb_context_make(stack.p, stack.size, MainEntry);
		}
		else if (stack.co != this->mRunContext->id)
		{
			this->SaveStack(stack.co);
			stack.co = this->mRunContext->id;
			Stack & curStack = this->mRunContext->stack;
			std::memcpy(this->mRunContext->ctx, curStack.p, curStack.size);
		}
#endif
		tb_context_from_t from = tb_context_jump(this->mRunContext->ctx, this);
		if (from.priv != nullptr)
		{
			this->mRunContext->ctx = from.ctx;
			return;
		}
#ifdef __ENABLE_SHARE_STACK__
		stack.co = 0;
#endif
		this->Remove(id);
		this->mRunContext = nullptr;
	}

	bool CoroutineComponent::YieldCoroutine()
	{
		assert(this->mRunContext);
		if(this->mRunContext == nullptr)
		{
			return false;
		}
		this->mRunContext->status = CorState::Suspend;
		tb_context_jump(this->mMainContext, this->mRunContext);
		return true;
	}

	void CoroutineComponent::Resume(unsigned int id)
	{
		TaskContext* coroutine = this->Get(id);
		if (coroutine == nullptr)
		{
			LOG_FATAL("try resume context id : {}", id);
			return;
		}
		if (coroutine->status == CorState::Ready || coroutine->status == CorState::Suspend)
		{
			this->mResumeContexts.emplace(coroutine);
			return;
		}
		LOG_FATAL("coroutine id:{} status:{}", id, (int)coroutine->status);
	}

	unsigned int CoroutineComponent::Invoke(std::unique_ptr<StaticMethod> & func)
	{
		//assert(this->mApp->IsMain());
		std::unique_ptr<TaskContext> coroutine;
		if (!this->mObjectPool.empty())
		{
			coroutine = std::move(this->mObjectPool.front());
			this->mObjectPool.pop();
		}
		else
		{
			coroutine = std::make_unique<TaskContext>();
#ifndef __ENABLE_SHARE_STACK__
			coroutine->stack.size = this->mConfig.stack;
#ifdef __ENABLE_MI_MALLOC__
			coroutine->stack.p =  (char*)mi_malloc(this->mConfig.stack);
#else
			coroutine->stack.p =  (char*)std::malloc(this->mConfig.stack);
#endif
#endif
		}
		this->mCount++;
		unsigned int coroutineId = this->mNumPool.BuildNumber();
		{
			coroutine->timerId = 0;
			coroutine->ctx = nullptr;
			coroutine->id = coroutineId;
			coroutine->cb = std::move(func);
			coroutine->status = CorState::Ready;
#ifdef __ENABLE_SHARE_STACK__
			coroutine->sid = this->mIndex++;
			if(this->mIndex >= this->mConfig.share)
			{
				this->mIndex = 0;
			}
			if(this->mSharedStack[coroutine->sid].co > 0)
			{
				for(unsigned int index = 0; index < this->mConfig.share; index++)
				{
					if(this->mSharedStack[index].co == 0)
					{
						coroutine->sid = index;
						break;
					}
				}
			}
#else
			Stack & stack = coroutine->stack;
			coroutine->ctx = tb_context_make(stack.p, stack.size, MainEntry);
			//LOG_DEBUG("stack size => {}", (stack.p + stack.size) - (char*)coroutine->ctx);
#endif
			this->mCoroutines.emplace(coroutineId, std::move(coroutine));
		}
		this->Resume(coroutineId);
		return coroutineId;
	}

	bool CoroutineComponent::YieldCoroutine(unsigned int& coroutineId)
	{
		if (this->mRunContext != nullptr)
		{
			coroutineId = this->mRunContext->id;
			return this->YieldCoroutine();
		}
		LOG_FATAL("not coroutine context");
		return false;
	}
#ifdef __ENABLE_SHARE_STACK__
	void CoroutineComponent::SaveStack(unsigned int id)
	{
		if (id == 0) return;
		TaskContext* coroutine = this->Get(id);
		if (coroutine == nullptr || coroutine->ctx == nullptr)
		{
			LOG_FATAL("coroutine context is null {}", id);
			return;
		}
		assert(coroutine->status == CorState::Suspend);
		const char* top = this->mSharedStack[coroutine->sid].top;
		ptrdiff_t size = (top - (const char*)coroutine->ctx);
		if (coroutine->stack.size < size)
		{
#ifdef __ENABLE_MI_MALLOC__
			void* newPtr = mi_realloc(coroutine->stack.p, size);
#else
			void* newPtr = realloc(coroutine->stack.p, size);
#endif
			if (newPtr == nullptr)
			{
				LOG_ERROR("alloc memory:{} is null", size);
				return;
			}
			coroutine->stack.size = size;
			coroutine->stack.p = (char*)newPtr;
		}
		std::memcpy(coroutine->stack.p, coroutine->ctx, size);
	}
#endif
	void CoroutineComponent::OnSystemUpdate(long long now)
	{
		while(!this->mResumeContexts.empty())
		{
			TaskContext * coroutine = this->mResumeContexts.front();
			{
				this->mResumeContexts.pop();
				if(coroutine == nullptr)
				{
					LOG_FATAL("not find task context");
					continue;
				}
			}
			this->RunCoroutine(coroutine);
		}
	}

	void CoroutineComponent::OnLastFrameUpdate()
	{
		while (!this->mLastQueues.empty())
		{
			unsigned int id = this->mLastQueues.front();
			TaskContext* coroutine = this->Get(id);
			if (coroutine != nullptr)
			{
				this->RunCoroutine(coroutine);
			}
			this->mLastQueues.pop();
		}
	}

	bool CoroutineComponent::Remove(unsigned int id)
	{
		auto iter = this->mCoroutines.find(id);
		if(iter == this->mCoroutines.end())
		{
			return false;
		}
		std::unique_ptr<TaskContext> coroutine = std::move(iter->second);
		{
			if(coroutine->timerId > 0)
			{
				this->mTimer->CancelTimer(coroutine->timerId);
				coroutine->timerId = 0;
			}
			if(this->mObjectPool.size() < this->mConfig.pool)
			{
				this->mObjectPool.emplace(std::move(coroutine));
			}

			this->mCoroutines.erase(iter);
		}
		return true;
	}

	TaskContext* CoroutineComponent::Get(unsigned int id) noexcept
	{
		auto iter = this->mCoroutines.find(id);
		return iter != this->mCoroutines.end() ? iter->second.get(): nullptr;
	}

	size_t CoroutineComponent::GetWaitCount() const
	{
		size_t count = 0;
		auto iter = this->mCoroutines.begin();
		for(; iter != this->mCoroutines.end(); iter++)
		{
			if(iter->second->status == CorState::Suspend)
			{
				count++;
			}
		}
		return count;
	}
}
