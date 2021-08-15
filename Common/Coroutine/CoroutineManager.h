﻿#pragma once

#include<list>
#include<queue>
#include<memory>
#include<tuple>
#include<functional>
#include<unordered_map>
#include"CoroutineEvent.h"

#ifndef _WIN32

#include<ucontext.h>

#endif

#include<Manager/Manager.h>
#include<NetWork/TcpClientSession.h>

#define CoroutinePoolMaxCount 100    //协程池最大数量
namespace Sentry
{
    struct Coroutine;

    class CoroutineManager : public Manager, public IFrameUpdate
    {
    public:
        CoroutineManager();

    public:
        long long Start(CoroutineAction func);

        long long Create(CoroutineAction func);

    public:
        void YieldReturn();

        void Sleep(long long ms);

        void Resume(long long id);

    protected:
        bool OnInit() final;

        void OnInitComplete() final;

        void OnFrameUpdate(float t) final;

    public:
        long long GetNowTime();

        void Destory(long long id);

        Coroutine *GetCoroutine();

        Coroutine *GetCoroutine(long long id);

        long long GetCurrentCorId()
        { return this->mCurrentCorId; }

        bool IsInMainCoroutine()
        { return this->mCurrentCorId == 0; }

        bool IsInLogicCoroutine()
        { return this->mCurrentCorId != 0; }

    private:
        void SaveStack(Coroutine *, char *top);

    private:
        std::string mMessageBuffer;

        class TimerManager *mTimerManager;

    private:
        long long mCurrentCorId;
#ifndef _WIN32
        ucontext_t mMainContext;
#else
        void *mMainCoroutineStack;
#endif
        int mCoroutinePoolMaxSize;  //协程池默认数量
        char mSharedStack[STACK_SIZE];
        std::queue<Coroutine *> mCoroutinePool;
        std::queue<Coroutine *> mDestoryCoroutine;
        std::unordered_map<long long, Coroutine *> mCoroutineMap;
    };
}