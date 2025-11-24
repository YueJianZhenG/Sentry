//
// Created by yjz on 2023/3/24.
//

#include "LuaSqlite.h"
#include "Entity/Actor/App.h"
#include "Yyjson/Lua/ljson.h"
#include "Util/Tools/TimeHelper.h"
#include "Sqlite/Component/SqliteComponent.h"
using namespace acs;
namespace lua
{
	inline int WriteResponse(lua_State * L, std::unique_ptr<sqlite::Response> response)
	{
		lua_createtable(L, 0, 2);
		{
			lua_pushstring(L, "ok");
			lua_pushboolean(L, response->ok);
			lua_rawset(L, -3);

			lua_pushstring(L, "count");
			lua_pushinteger(L, response->count);
			lua_rawset(L, -3);

			if(!response->error.empty())
			{
				lua_pushstring(L, "error");
				lua_pushlstring(L, response->error.c_str(), response->error.size());
				lua_rawset(L, -3);
			}

			if (!response->result.empty())
			{
				int index = 0;
				lua_pushstring(L, "list");
				lua_createtable(L, 0, (int)response->result.size());
				for (const std::unique_ptr<json::r::Document>& json: response->result)
				{
					lua_pushinteger(L, ++index);
					lua::yyjson::write(L, json->GetValue());
					lua_settable(L, -3);
				}
				lua_rawset(L, -3);
			}
		}
		return 1;
	}

	inline SqliteComponent * GetComponent()
	{
		static SqliteComponent * sqliteComponent = nullptr;
		if(sqliteComponent == nullptr)
		{
			sqliteComponent = App::Get<SqliteComponent>();
			if(sqliteComponent == nullptr)
			{
				return nullptr;
			}
		}
		return sqliteComponent;
	}

	int Sqlite::Get(lua_State* L)
	{
		size_t count = 0;
		const char * k = luaL_checklstring(L, 1, &count);

		std::string key(k, count);
		SqliteComponent * sqliteComponent = GetComponent();
		std::unique_ptr<sqlite::Response> response = sqliteComponent->Invoke("local_data_select", key);
		if(!response->ok || response->result.empty())
		{
			return 0;
		}
		std::unique_ptr<json::r::Document> & document = response->result.front();
		{
			json::r::Value jsonValue;
			if(!document->Get("content", jsonValue))
			{
				return 0;
			}
			long long expTime = 0;
			document->Get("exp_time", expTime);
			if(expTime > 0 && help::Time::NowSec() >= expTime)
			{
				sqliteComponent->Del(key);
				return 0;
			}
			if(jsonValue.IsObject() || jsonValue.IsArray())
			{
				std::string json = jsonValue.ToString();
				lua::yyjson::write(L, jsonValue.GetValue());
				return 1;
			}
			size_t count = 0;
			const char * str = jsonValue.GetString(count);
			lua_pushlstring(L, str, count);
		}
		return 1;
	}

	int Sqlite::Set(lua_State* L)
	{
		size_t count = 0;
		const char * k = luaL_checklstring(L, 1, &count);

		std::string value;
		switch(lua_type(L, 2))
		{
			case LUA_TTABLE:
			{
				if(!yyjson::read(L, 2, value))
				{
					LOG_ERROR("lua table cast json fail");
					return 0;
				}
				break;
			}
			case LUA_TSTRING:
			{
				size_t count = 0;
				const char * str = lua_tolstring(L, 2, &count);
				if(str != nullptr && count > 0)
				{
					value.assign(str, count);
				}
				break;
			}
			default:
			{
				LOG_ERROR("args must string or table");
				return 0;
			}
		}
		std::string key(k, count);
		SqliteComponent * sqliteComponent = GetComponent();
		lua_pushboolean(L, sqliteComponent->Set(key, value));
		return 1;
	}

	int Sqlite::Del(lua_State* L)
	{
		size_t count = 0;
		const char * k = luaL_checklstring(L, 1, &count);
		{
			std::string key(k, count);
			SqliteComponent * sqliteComponent = GetComponent();
			lua_pushboolean(L, sqliteComponent->Del(key));
		}
		return 1;
	}

	int Sqlite::SetTimeout(lua_State* L)
	{
		size_t count = 0;
		const char * k = luaL_checklstring(L, 1, &count);
		int timeout = (int)luaL_checkinteger(L, 2);
		{
			std::string key(k, count);
			SqliteComponent * sqliteComponent = GetComponent();
			lua_pushboolean(L, sqliteComponent->SetTimeout(key, timeout));
			return 1;
		}

	}

	int Sqlite::Run(lua_State* L)
	{
		SqliteComponent * sqliteComponent = GetComponent();

		size_t size = 0;
		const char * sql = luaL_checklstring(L, 1, &size);
		return lua::WriteResponse(L, sqliteComponent->Run(sql, size));
	}

	int Sqlite::Build(lua_State* L)
	{
		size_t count, count2 = 0;
		const char * str1 = luaL_checklstring(L, 1, &count2);
		const char * sql = luaL_checklstring(L, 2, &count);

		std::string name(str1, count2);
		SqliteComponent * sqliteComponent = GetComponent();
		bool result = sqliteComponent->Build(name, sql, count);
		lua_pushboolean(L, result);
		return 1;
	}

	int Sqlite::Invoke(lua_State* L)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 1, &count);

		std::string name(str, count);
		SqliteComponent * sqliteComponent = GetComponent();
		return lua::WriteResponse(L, sqliteComponent->Invoke(name, L));
	}
}
