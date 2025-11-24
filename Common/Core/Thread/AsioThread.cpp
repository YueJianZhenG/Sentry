//
// Created by leyi on 2023/8/8.
//

#include "AsioThread.h"

#include "Log/Common/Debug.h"
#include "Util/Tools/TimeHelper.h"
namespace custom
{
	AsioThread::AsioThread(int update)
		:mContext(1), mUpdate(update)
	{
		this->mId = 0;
		this->mCount = 0;
		this->mLastTime = 0;
#ifdef __OS_WIN__
		this->mThreadHandler = nullptr;
#else

#endif
	}

	void AsioThread::Stop()
	{
		Asio::Code code;
		this->mContext.stop();
		if(this->mThread.joinable())
		{
			this->mThread.join();
		}
	}

	void AsioThread::ReStart()
	{
		this->mContext.restart();
	}

	void AsioThread::GetBacktrace(std::string& backtrace)
	{
#ifdef __OS_WIN__
		Debug::Backtrace(backtrace, this->mThreadHandler);
#else

#endif
	}


	void AsioThread::Start(int id, const std::string & name)
	{
		this->mId = id;
		this->mName = name;
		std::thread(&AsioThread::Run, this).swap(this->mThread);
	}

	void AsioThread::Run()
	{
#ifdef __OS_WIN__
		this->mThreadHandler = GetCurrentThread();
#else

#endif
		std::chrono::seconds sleep(this->mUpdate);
		auto work = asio::make_work_guard(this->mContext);
		while (!this->mContext.stopped())
		{
			this->mLastTime = help::Time::NowSec();
			this->mCount += this->mContext.run_for(sleep);
		}
//		while(!this->mContext.stopped())
//		{
//			this->mLastTime = help::Time::NowSec();
//			this->mContext.run_one_for(std::chrono::seconds(5));
//			//printf("[%s:%d] invoke once event\n", this->mName.c_str(), this->mId);
//		}
	}
}