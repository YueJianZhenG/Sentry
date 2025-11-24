#pragma once
#include <unordered_map>
#include "Rpc/Method/MethodProxy.h"
#include "Timer/Timer/TimeWheelLayer.h"
#include "Entity/Component/Component.h"

//定时器类型 决定了定时器ID
namespace timer
{
	constexpr int TIMER_TYPE_NONE = 0;
	constexpr int TIMER_TYPE_RPC = 1;
	constexpr int TIMER_TYPE_MYSQL = 2;
	constexpr int TIMER_TYPE_PGSQL = 3;
	constexpr int TIMER_TYPE_MONGO = 4;
	constexpr int TIMER_TYPE_REDIS = 5;
}

namespace acs
{
	class TimerComponent final : public Component, public ISystemUpdate,
								 public ILastFrameUpdate, public IServerRecord, public IStart, public IDestroy
	{
	 public:
		TimerComponent();
	 public:
		bool CancelTimer(long long id);

		long long Timeout(int ms, std::function<void(void)> && callback);

		template<typename F, typename O, typename ... Args>
		long long Timeout(int ms, F&& f, O* o, Args&& ... args)
		{
			std::unique_ptr<StaticMethod> methodProxy = NewMethodProxy(
				std::forward<F>(f), o, std::forward<Args>(args)...);
			return this->CreateTimer(ms, std::move(methodProxy));
		}
		template<typename F, typename O, typename ... Args>
		long long Timeout(long long id, int ms, F&& f, O* o, Args&& ... args)
		{
			std::unique_ptr<StaticMethod> methodProxy = NewMethodProxy(
					std::forward<F>(f), o, std::forward<Args>(args)...);
			return this->CreateTimer(id, ms, std::move(methodProxy));
		}
	public:
		static long long MakeTimerID();
		static long long MakeTimerID(int type, int id);
		bool AddTimer(std::unique_ptr<TimerBase> timer);
		long long CreateTimer(int ms, std::unique_ptr<StaticMethod> func);
		long long CreateTimer(long long id, int ms, std::unique_ptr<StaticMethod> func);
	private:
		void OnTick();
		bool Awake() final;
		void OnStart() final;
		void OnDestroy() final;
		void OnLastFrameUpdate() final;
		bool InvokeTimer(long long timerId);
		bool AddTimerToWheel(long long timerId);
		void OnSystemUpdate(long long nowMs) final;
		void OnRecord(json::w::Document &document) final;
		bool AddTimerToWheel(std::unique_ptr<TimerBase> timer);
	private:
		void OnHour();
		void OnNewDay();
		void OnMinute();
	 private:
		int mIndex;
		int mDoneCount;
		int mCancelCount;
		long long mNowTime;
		long long mNextUpdateTime;
		std::queue<long long> mDelTimers;
		std::queue<long long> mTriggerTimers;
		std::vector<TimeWheelLayer> mTimerLayers;
		std::queue<long long> mLastFrameTriggerTimers;
		std::unordered_map<long long, std::unique_ptr<TimerBase>> mTimerMap;//所有timer的列表
	};
}// namespace Sentry