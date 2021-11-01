﻿#pragma once

#include"RedisDefine.h"
#include"RedisTaskBase.h"
#include<Script/LuaInclude.h>

#define RedisLuaArgvSize 10
namespace GameKeeper
{
    class QuertJsonWritre;

    class RedisLuaTask : public RedisTaskBase
    {
    public:
        RedisLuaTask(const std::string &cmd, lua_State *lua, int ref);

        ~RedisLuaTask();

    protected:
        void RunFinish() final;  //执行完成之后在主线程调用
    public:
        static RedisLuaTask * Create(lua_State *lua, int index, const char *cmd);

    private:
        int mCoroutienRef;
        lua_State *mLuaEnv;
    private:
        std::string mQueryJsonData;
    };
}