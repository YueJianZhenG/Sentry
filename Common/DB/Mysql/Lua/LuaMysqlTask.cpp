//
// Created by yjz on 2023/3/24.
//



#include"LuaMysqlTask.h"
#include"Lua/Engine/Define.h"
#include "Yyjson/Lua/ljson.h"
namespace acs
{
	LuaMysqlTask::LuaMysqlTask(lua_State *lua, int id)
		: IRpcTask<mysql::Response>(id), mLua(lua)
	{
		this->mRef = 0;
		if(lua_isthread(this->mLua, -1))
		{
			this->mRef = luaL_ref(lua, LUA_REGISTRYINDEX);
		}
	}
	LuaMysqlTask::~LuaMysqlTask()
	{
		if(this->mRef != 0)
		{
			luaL_unref(this->mLua, LUA_REGISTRYINDEX, this->mRef);
		}
	}

	int LuaMysqlTask::Await()
	{
		if(this->mRef == 0)
		{
			luaL_error(this->mLua, "not lua coroutine context yield failure");
			return 0;
		}
		return lua_yield(this->mLua, 0);
	}

	inline void lua_push_value(lua_State * L, const char * key, bool value)
	{
		lua_pushstring(L, key);
		lua_pushboolean(L, value);
		lua_rawset(L, -3);
	}

	inline void lua_push_value(lua_State * L, const char * key, unsigned int value)
	{
		lua_pushstring(L, key);
		lua_pushinteger(L, value);
		lua_rawset(L, -3);
	}
	inline void lua_push_value(lua_State * L, const char * key, const std::string & value)
	{
		lua_pushstring(L, key);
		lua_pushlstring(L, value.c_str(), value.size());
		lua_rawset(L, -3);
	}

	void LuaMysqlTask::OnTimeout()
	{
		std::unique_ptr<mysql::Response> response = std::make_unique<mysql::Response>(mysql::PACKAGE_ERR);
		{
			response->error.emplace_back("time out");
			this->OnResponse(response);
		}
	}

	void LuaMysqlTask::OnResponse(std::unique_ptr<mysql::Response>& response) noexcept
	{
		lua_rawgeti(this->mLua, LUA_REGISTRYINDEX, this->mRef);
		lua_State* coroutine = lua_tothread(this->mLua, -1);

		lua_newtable(this->mLua);
		{
			bool ok = response->IsOk();
			lua_push_value(this->mLua, "ok", ok);
			switch(response->GetPackageCode())
			{
				case mysql::PACKAGE_OK:
				{

					lua_pushstring(this->mLua, "result");
					lua_createtable(this->mLua, 0, 4);
					{
						lua_push_value(this->mLua, "warn_count", response->ok.mWarningCount);
						lua_push_value(this->mLua, "server_status", response->ok.mServerStatus);
						lua_push_value(this->mLua, "affected_rows", response->ok.mAffectedRows);
						lua_push_value(this->mLua, "last_insert_id", response->ok.mLastInsertId);
					}
					lua_rawset(this->mLua, -3);
					break;
				}
				case mysql::PACKAGE_ERR:
				{
					lua_pushstring(this->mLua, "result");
					lua_createtable(this->mLua, 0, 2);
					{
						lua_push_value(this->mLua, "error", response->GetBuffer());
						lua_push_value(this->mLua, "code", (unsigned int)response->GetErrorCode());
					}
					lua_rawset(this->mLua, -3);
					break;
				}
				default:
				{

					int index = 1;
					lua_pushstring(this->mLua, "result");
					lua_createtable(this->mLua, 0, response->contents.size());
					for (const std::string& json: response->contents)
					{
						lua::yyjson::write(this->mLua, json.c_str(), json.size());
						lua_seti(this->mLua, -2, index++);
					}
					lua_rawset(this->mLua, -3);

					break;
				}
			}
		}
		Lua::Coroutine::Resume(coroutine, 1);
	}
}
