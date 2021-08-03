﻿#include "RedisTaskBase.h"
#include <Manager/RedisManager.h>

namespace Sentry
{

    RedisTaskBase::RedisTaskBase(RedisManager *mgr, const std::string &cmd)
    {
        this->mRedisManager = mgr;
        this->mCommand.push_back(cmd);
    }

    void RedisTaskBase::InvokeInThreadPool(long long threadId)
    {
        RedisSocket *redisSocket = this->mRedisManager->GetRedisSocket(threadId);
        if (redisSocket == nullptr)
        {
            this->mErrorStr = "redis scoket null";
            this->mErrorCode = XCode::RedisSocketIsNull;
            return;
        }
        const char **argvArray = new const char *[this->mCommand.size()];
        size_t *argvSizeArray = new size_t[this->mCommand.size()];

        for (size_t index = 0; index < this->mCommand.size(); index++)
        {
            argvArray[index] = this->mCommand[index].c_str();
            argvSizeArray[index] = this->mCommand[index].size();
        }
        const int size = (int) this->mCommand.size();
        redisReply *replay = (redisReply *) redisCommandArgv(redisSocket, size, argvArray, argvSizeArray);

        delete[] argvArray;
        delete[] argvSizeArray;

        //redisReply * replay = (redisReply*)redisvCommand(redisSocket, this->mFormat.c_str(), this->mCommand);
        if (replay == nullptr)
        {
            this->mErrorStr = "redis replay null";
            this->mErrorCode = XCode::RedisReplyIsNull;
            return;
        }
        switch (replay->type)
        {
            case REDIS_REPLY_STATUS:
                this->mErrorCode = XCode::Successful;
                this->mQueryDatas.push_back(std::string(replay->str, replay->len));
                break;
            case REDIS_REPLY_ERROR:
                this->mErrorCode = RedisInvokeFailure;
                this->mErrorStr.assign(replay->str, replay->len);
                break;
            case REDIS_REPLY_INTEGER:
                this->mErrorCode = XCode::Successful;
                this->mQueryDatas.push_back(std::to_string(replay->integer));
                break;
            case REDIS_REPLY_NIL:
                this->mErrorCode = XCode::Successful;
                break;
            case REDIS_REPLY_STRING:
                this->mErrorCode = XCode::Successful;
                this->mQueryDatas.push_back(std::to_string(replay->integer));
                break;
            case REDIS_REPLY_ARRAY:
                this->mErrorCode = XCode::Successful;
                for (size_t index = 0; index < replay->elements; index++)
                {
                    redisReply *redisData = replay->element[index];
                    if (redisData->type == REDIS_REPLY_INTEGER || redisData->type == REDIS_REPLY_STRING)
                    {
                        this->mQueryDatas.push_back(std::to_string(redisData->integer));
                    }
                }
                break;
        }
        freeReplyObject(replay);
    }

    void RedisTaskBase::AddCommandArgv(const std::string &argv)
    {
        this->mCommand.push_back(argv);
    }

    void RedisTaskBase::AddCommandArgv(const char *str, const size_t size)
    {
        this->mCommand.push_back(std::string(str, size));
    }

    bool RedisTaskBase::GetOnceData(std::string &value)
    {
        if (this->mErrorCode != XCode::Successful)
        {
            return false;
        }
        if (this->mQueryDatas.empty())
        {
            return false;
        }
        value = this->mQueryDatas[0];
        return true;
    }
}// namespace Sentry