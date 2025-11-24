//
// Created by yy on 2024/10/9.
//

#include "LuaSmsTask.h"

namespace acs
{
	LuaSmsRequestTask::LuaSmsRequestTask(int id, lua_State* lua)
		:IRpcTask<http::Response>(id), mLua(lua), mRef(0)
	{
		if(lua_isthread(this->mLua, -1))
		{
			this->mRef = luaL_ref(lua, LUA_REGISTRYINDEX);
		}
	}

	LuaSmsRequestTask::~LuaSmsRequestTask() noexcept
	{
		if(this->mRef != 0)
		{
			luaL_unref(this->mLua, LUA_REGISTRYINDEX, this->mRef);
		}
	}

	int LuaSmsRequestTask::Await() noexcept
	{
		if(this->mRef == 0)
		{
			luaL_error(this->mLua, "not lua coroutine context yield failure");
			return 0;
		}
		return lua_yield(this->mLua, 0);
	}

	void LuaSmsRequestTask::OnResponse(std::unique_ptr<http::Response> & response) noexcept
	{
		int count = 0;
		if(response != nullptr && response->Code() == HttpStatus::OK)
		{
			std::unique_ptr<http::Content> httpContent = response->MoveBody();
			if(httpContent)
			{
				count++;
				httpContent->WriteToLua(this->mLua);
			}
		}
		Lua::Coroutine::Resume(this->mLua, count);
	}

	void LuaSmsRequestTask::OnTimeout()
	{
		std::unique_ptr<http::Response> response =
				std::make_unique<http::Response>();
		response->SetCode(HttpStatus::REQUEST_TIMEOUT);

		this->OnResponse(response);
	}
}