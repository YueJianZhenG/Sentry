#pragma once

#include<queue>
#include<memory>
#include<functional>
#include"Entity/Component/Component.h"
#include"Async/Coroutine/TaskContext.h"
#include "Yyjson/Object/JsonObject.h"
namespace coroutine
{
	struct Config : public json::Object<Config>
	{
	public:
		unsigned int pool;  //对象池
#ifdef __ENABLE_SHARE_STACK__
		unsigned int share; //共享栈数量 kb
#endif
		unsigned int stack; //栈大小 kb
	};
}

namespace acs
{
	class CoroutineComponent final : public Component, public IRefresh,
									 public ISystemUpdate, public ILastFrameUpdate, public IServerRecord
	{
	public:
		CoroutineComponent();
#ifdef __ENABLE_SHARE_STACK__
		~CoroutineComponent() override;
#endif
	public:
		template<typename F, typename T, typename ... Args>
		inline unsigned int Start(F&& f, T* o, Args&& ... args) noexcept
		{
			std::unique_ptr<StaticMethod> method = NewMethodProxy(std::forward<F>(f), o, std::forward<Args>(args)...);
			{
				return this->Invoke(method);
			}
		}
		inline unsigned int Start(std::function<void()>&& func) noexcept
		{
			std::unique_ptr<StaticMethod> method = std::make_unique<LambdaMethod>(std::move(func));
			{
				return this->Invoke(method);
			}
		}
	public:
		bool WaitNextFrame();
		bool YieldCoroutine();
		bool Sleep(unsigned int ms);
		void Resume(unsigned int id);
		bool SetTimeout(int timeout);
		bool YieldCoroutine(unsigned int& coroutineId);
		inline size_t Count() const { return this->mCoroutines.size(); }
	private:
		bool Awake() final;
		bool OnRefresh() final;
		bool LateAwake() final;
		void OnLastFrameUpdate() final;
		void OnSystemUpdate(long long now) final;
		void OnRecord(json::w::Document &document) final;
	public:
		void RunTask(tb_context_t context);
		inline unsigned int GetContextId() const
		{
			return this->mRunContext == nullptr ? 0 :
				   this->mRunContext->id;
		}
	private:
		size_t GetMemory();
		size_t GetWaitCount() const;
		bool Remove(unsigned int id);
#ifdef __ENABLE_SHARE_STACK__
		void SaveStack(unsigned int id);
#endif
		TaskContext* Get(unsigned int id) noexcept;
		void RunCoroutine(TaskContext * coroutine);
		unsigned int Invoke(std::unique_ptr<StaticMethod> & func);
	private:
		size_t mCount;
		size_t mIndex;
		TaskContext* mRunContext;
		tb_context_t mMainContext;
		coroutine::Config mConfig;
		class TimerComponent * mTimer;
		std::queue<unsigned int> mLastQueues;
#ifdef __ENABLE_SHARE_STACK__
		std::unique_ptr<Stack[]> mSharedStack;
#endif
		math::NumberPool<unsigned int> mNumPool;
		std::queue<TaskContext  *> mResumeContexts;
		std::queue<std::unique_ptr<TaskContext>> mObjectPool;
		std::unordered_map<unsigned int, std::unique_ptr<TaskContext>> mCoroutines;
	};
}