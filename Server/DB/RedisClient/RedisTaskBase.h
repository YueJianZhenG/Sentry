﻿#pragma once

#include "RedisDefine.h"
#include <Thread/TaskProxy.h>
#include <XCode/XCode.h>
#include <memory>
#include <string>
#include <vector>

#define RedisLuaArgvSize 10
namespace Sentry
{
    class RedisComponent;

    class QuertJsonWritre;

    class RedisTaskBase : public TaskProxy
    {
    public:
        RedisTaskBase(const std::string &cmd);
    public:
        bool Run() final;//在线程池执行的任务
    public:
        template<typename... Args>
        void InitCommand(Args &&...args);
        float GetCostTime();

    private:
        //inline void AddCommand(const char *value) { this->mCommand.push_back(value); }

        inline void AddCommand(const std::string &value) { this->mCommand.push_back(value); }

        inline void AddCommand(const int value) { this->mCommand.push_back(std::to_string(value)); }

        inline void AddCommand(const float value) { this->mCommand.push_back(std::to_string(value)); }

        inline void AddCommand(const double value) { this->mCommand.push_back(std::to_string(value)); }

        inline void AddCommand(const long long value) { this->mCommand.push_back(std::to_string(value)); }

    private:
        void Encode() {}

        template<typename T, typename... Args>
        inline void Encode(const T &t, Args... args)
        {
            this->AddCommand(t);
            this->Encode(std::forward<Args>(args)...);
        }

    public:
        void AddCommandArgv(const std::string &argv);

        void AddCommandArgv(const char *str, const size_t size);

    public:

        void DebugInvokeInfo();

        bool GetOnceData(std::string &value);

        XCode GetErrorCode() { return this->mErrorCode; }

        const std::string &GetErrorStr() { return this->mErrorStr; }

        std::vector<std::string> &GetQueryDatas() { return this->mQueryDatas; }

    private:
        long long mStartTime;
        std::vector<std::string> mCommand;
    private:
        XCode mErrorCode;
        std::string mErrorStr;

    private:       
        std::vector<std::string> mQueryDatas;
    };

    template<typename... Args>
    inline void RedisTaskBase::InitCommand(Args &&...args)
    {
        this->Encode(std::forward<Args>(args)...);
    }
}// namespace Sentry