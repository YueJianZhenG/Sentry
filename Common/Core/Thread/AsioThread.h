//
// Created by leyi on 2023/8/8.
//

#ifndef APP_ASIOTHREAD_H
#define APP_ASIOTHREAD_H
#include<thread>
#include"Network/Tcp/Asio.h"

namespace custom
{
	class AsioThread
	{
	public:
		explicit AsioThread(int update = 10);
		AsioThread(const AsioThread & t) = delete;
		AsioThread(const AsioThread && t) = delete;
	public:
		void Stop();
		void ReStart();
		void GetBacktrace(std::string & backtrace);
		void Start(int id, const std::string & name);
		Asio::Context & Context() { return mContext; }
		inline int GetId() const { return this->mId; }
		const std::string & Name() const { return this->mName; }
		inline size_t GetEventCount() const { return this->mCount; }
		inline long long GetLastTime() const { return this->mLastTime; }
	private:
		void Run();
	private:
		int mId;
		int mUpdate;
		size_t mCount;
		std::string mName;
		std::thread mThread;
		long long mLastTime;
#ifdef __OS_WIN__
		void * mThreadHandler;
#else

#endif
		Asio::Context mContext;
	};
}

#endif //APP_ASIOTHREAD_H
