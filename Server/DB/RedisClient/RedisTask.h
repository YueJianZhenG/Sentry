﻿#pragma once

#include"RedisTaskBase.h"
#include<QueryResult/InvokeResultData.h>


namespace Sentry
{
    class RedisInvokeResult
    {
    public:

    };
}

namespace Sentry
{
    class CoroutineComponent;

    class RedisTask : public RedisTaskBase
    {
    public:
        RedisTask(const std::string &cmd);

    public:
        long long GetCoroutineId() { return mCoreoutineId; }
        
    protected:
        void RunFinish() final;  //执行完成之后在主线程调用
    private:
        unsigned int mCoreoutineId;       
    };
}