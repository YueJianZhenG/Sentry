#include "Table.h"
#include "Util/File/FileHelper.h"
namespace Lua
{
	Table::Table(lua_State* luaEnv, int ref)
		: ref(ref), mLuaEnv(luaEnv)
	{

	}

	Table::~Table()
	{
		luaL_unref(mLuaEnv, LUA_REGISTRYINDEX, this->ref);
	}

	std::unique_ptr<Function> Table::GetFunction(const std::string& name)
	{
		lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
		lua_getfield(this->mLuaEnv, -1, name.c_str());
		if (!lua_isfunction(this->mLuaEnv, -1))
		{
			return nullptr;
		}
		int ref1 = luaL_ref(this->mLuaEnv, LUA_REGISTRYINDEX);
		return std::make_unique<Function>(this->mLuaEnv, ref1);
	}

	std::unique_ptr<Table> Table::GetTable(int index)
	{
		lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
		if (!lua_istable(this->mLuaEnv, -1))
		{
			return nullptr;
		}
		lua_rawgeti(this->mLuaEnv, -1, index);
		if (!lua_istable(this->mLuaEnv, -1))
		{
			return nullptr;
		}
		int ref = luaL_ref(this->mLuaEnv, LUA_REGISTRYINDEX);
		return std::make_unique<Table>(this->mLuaEnv, ref);
	}

	std::unique_ptr<Table> Table::GetTable(const std::string& name)
	{
		lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, this->ref);
		lua_getfield(this->mLuaEnv, -1, name.c_str());
		if (!lua_istable(this->mLuaEnv, -1))
		{
			return nullptr;
		}
		return std::make_unique<Table>(this->mLuaEnv, -1);
	}

	std::unique_ptr<Table> Table::Create(lua_State* luaEnv, const std::string& name)
	{
		if(!help::fs::FileIsExist(name.c_str()))
		{
			lua_getglobal(luaEnv, name.c_str());
		}
		else
		{
			luaL_dofile(luaEnv, name.c_str());
		}
		if (!lua_istable(luaEnv, -1))
		{
			return nullptr;
		}
		int ref = luaL_ref(luaEnv, LUA_REGISTRYINDEX);
		return std::make_unique<Table>(luaEnv, ref);
	}

	bool Table::Get(lua_State * lua, const std::string & name)
	{
		lua_getglobal(lua, name.c_str());
		return lua_istable(lua, -1);
	}
}
