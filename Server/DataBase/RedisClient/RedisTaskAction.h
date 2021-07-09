﻿#pragma once
#include"RedisTaskBase.h"
#include<QueryResult/InvokeResultData.h>


namespace SoEasy
{
	class RedisInvokeResult
	{
	public:

	};
}

namespace SoEasy
{
	class CoroutineManager;
	class RedisTaskAction : public RedisTaskBase
	{
	public:
		RedisTaskAction(RedisManager * mgr, long long taskId, const std::string & cmd);
	public:
		long long GetCoroutineId() { return mCoreoutineId; }
	protected:
		void OnTaskFinish() final;  //执行完成之后在主线程调用
	private:
		long long mCoreoutineId;
		CoroutineManager * mCorManager;
	};
	typedef std::shared_ptr<RedisTaskAction> RedisSharedTask;
}