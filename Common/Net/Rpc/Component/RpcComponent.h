//
// Created by zmhy0073 on 2022/6/20.
//

#ifndef APP_RPCCOMPONENT_H
#define APP_RPCCOMPONENT_H
#include <queue>
#include <unordered_map>
#include "Rpc/Async/RpcTaskSource.h"
#include "Entity/Component/Component.h"
#include "Timer/Component/TimerComponent.h"
namespace acs
{
    template<typename T>
	class RpcComponent : public Component, public ILastFrameUpdate
    {
    public:
        RpcComponent() = default;
        ~RpcComponent() override = default;
        typedef IRpcTask<T> * RpcTask;
    public:
        template<typename T1>
		inline T1 * AddTask(T1 * task, int timeout = 0)
        {
			static TimerComponent * timerComponent = this->GetComponent<TimerComponent>();
			{
				int rpcId = task->GetRpcId();
				auto iter = this->mTasks.find(rpcId);
				if(iter != this->mTasks.end())
				{
					this->mDelTasks.emplace(task);
					LOG_ERROR("add task already exist");
					return nullptr;
				}
				if(timeout > 0)
				{
					int ms = timeout * 1000;
					long long id = timerComponent->Timeout(ms,
							&RpcComponent<T>::OnTimeout, this, rpcId);
					task->SetTimerId(id);
				}
				this->mTasks.emplace(rpcId, task);
			}
            return task;
        }
		template<typename T1>
		inline T1 * BuildRpcTask(int rpcId, unsigned int timeout = 0)
		{
			T1 * task = new T1(rpcId);
			return this->AddTask(task, timeout);
		}

		inline void OnTimeout(int key);
		inline bool OnResponse(int key, std::unique_ptr<T>& message);
		inline size_t AwaitCount() const { return this->mTasks.size(); }
		inline int BuildRpcId() { return this->mNumberPool.BuildNumber(); }
		inline long long CurrentRpcCount() { return this->mNumberPool.CurrentNumber(); }
	protected:
		void OnLastFrameUpdate() noexcept final;
        virtual void OnNotFindResponse(int key, std::unique_ptr<T>& message);
    private:
		std::queue<RpcTask> mDelTasks;
		math::NumberPool<int> mNumberPool;
		std::unordered_map<int, RpcTask> mTasks;
    };

	template<typename T>
	void RpcComponent<T>::OnLastFrameUpdate() noexcept
	{
		while(!this->mDelTasks.empty())
		{
			delete this->mDelTasks.front();
			this->mDelTasks.pop();
		}
	}

	template<typename T>
	inline void RpcComponent<T>::OnTimeout(int key)
	{
		static TimerComponent* timerComponent = this->GetComponent<TimerComponent>();
		{
			auto iter1 = this->mTasks.find(key);
			if (iter1 == this->mTasks.end())
			{
				return;
			}

			iter1->second->OnTimeout();
			long long timerId = iter1->second->GetTimerId();
			timerComponent->CancelTimer(timerId);
			this->mDelTasks.emplace(iter1->second);
			this->mTasks.erase(iter1);
		}
	}

    template<typename T>
    inline bool RpcComponent<T>::OnResponse(int key, std::unique_ptr<T>& message)
	{
		static TimerComponent* timerComponent = this->GetComponent<TimerComponent>();
		{
			auto iter1 = this->mTasks.find(key);
			if (iter1 == this->mTasks.end())
			{
				this->OnNotFindResponse(key, message);
				return false;
			}

			iter1->second->OnResponse(message);
			long long timerId = iter1->second->GetTimerId();
			timerComponent->CancelTimer(timerId);
			this->mDelTasks.emplace(iter1->second);
			this->mTasks.erase(iter1);
		}
		return true;
	}

    template<typename T>
    void RpcComponent<T>::OnNotFindResponse(int k, std::unique_ptr<T>& message)
    {
        LOG_ERROR("{} not find rpc task id({}) ", this->GetName(), k);
    }
}


#endif //APP_RPCCOMPONENT_H
