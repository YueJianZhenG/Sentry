//
// Created by yy on 2024/10/9.
//

#include "LuaOssTask.h"

namespace acs
{
	LuaOssRequestTask::LuaOssRequestTask(int id, lua_State* lua, std::string url)
		:IRpcTask<http::Response>(id), mLua(lua), mRef(0), mUrl(std::move(url))
	{
		if(lua_isthread(this->mLua, -1))
		{
			this->mRef = luaL_ref(lua, LUA_REGISTRYINDEX);
		}
	}

	LuaOssRequestTask::~LuaOssRequestTask() noexcept
	{
		if(this->mRef != 0)
		{
			luaL_unref(this->mLua, LUA_REGISTRYINDEX, this->mRef);
		}
	}

	int LuaOssRequestTask::Await() noexcept
	{
		if(this->mRef == 0)
		{
			luaL_error(this->mLua, "not lua coroutine context yield failure");
			return 0;
		}
		return lua_yield(this->mLua, 0);
	}

	void LuaOssRequestTask::OnResponse(std::unique_ptr<http::Response> & response) noexcept
	{
		lua_newtable(this->mLua);
		{
			lua_createtable(this->mLua, 0, 4);
			Lua::push_value(this->mLua, "code", response->Code());
			Lua::push_value(this->mLua, "status", response->GetError());

			const http::Head & head = response->Header();
			lua_createtable(this->mLua, 0, (int)head.Count());
			for(const std::pair<std::string, std::string> & iter : head.GetValue())
			{
				const std::string & key = iter.first;
				const std::string & value = iter.second;
				lua_pushlstring(this->mLua, key.c_str(), key.size());
				lua_pushlstring(this->mLua, value.c_str(), value.size());
				lua_rawset(this->mLua, -3);
			}
		}
		lua_setfield(this->mLua, -2, "result");
		if(response->Code() == HttpStatus::OK)
		{
			lua_pushlstring(this->mLua, this->mUrl.c_str(), this->mUrl.size());
			lua_setfield(this->mLua, -2, "url");
		}
		Lua::Coroutine::Resume(this->mLua, 1);
	}

	void LuaOssRequestTask::OnTimeout()
	{
		std::unique_ptr<http::Response> response =
				std::make_unique<http::Response>();
		response->SetCode(HttpStatus::REQUEST_TIMEOUT);

		this->OnResponse(response);
	}
}