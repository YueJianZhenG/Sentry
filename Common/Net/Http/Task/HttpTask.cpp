//
// Created by zmhy0073 on 2022/11/2.
//

#include"HttpTask.h"

namespace acs
{
    LuaHttpRequestTask::LuaHttpRequestTask(int id, lua_State *lua)
        : IRpcTask<http::Response>(id), mRef(0), mLua(lua)
    {
        if(lua_isthread(this->mLua, -1))
        {
            this->mRef = luaL_ref(lua, LUA_REGISTRYINDEX);
        }
    }

    LuaHttpRequestTask::~LuaHttpRequestTask()
    {
        if(this->mRef != 0)
        {
            luaL_unref(this->mLua, LUA_REGISTRYINDEX, this->mRef);
        }
    }

    void LuaHttpRequestTask::OnResponse(std::unique_ptr<http::Response>& response) noexcept
    {
        int count = 0;
        if(response != nullptr)
        {
            count = response->WriteToLua(this->mLua);
        }
        Lua::Coroutine::Resume(this->mLua, count);
    }

	void LuaHttpRequestTask::OnTimeout()
	{
		std::unique_ptr<http::Response> response = std::make_unique<http::Response>();
		{
			response->SetCode(HttpStatus::REQUEST_TIMEOUT);
			response->WriteToLua(this->mLua);
		}
		Lua::Coroutine::Resume(this->mLua, 1);
	}

    int LuaHttpRequestTask::Await() noexcept
    {
        if(this->mRef == 0)
        {
            LOG_ERROR("not lua coroutine context yield failure");
            return 0;
        }
        return lua_yield(this->mLua, 0);
    }
}