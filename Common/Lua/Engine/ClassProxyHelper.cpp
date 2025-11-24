#include "ClassProxyHelper.h"

namespace Lua
{
	ClassProxyHelper::ClassProxyHelper(lua_State* lua, const std::string& name)
		: mLua(lua), mName(name)
	{

	}

	void ClassProxyHelper::PushGlobalExtensionFunction(const char* name, lua_CFunction func)
	{
		lua_pushcfunction(this->mLua, func);
		lua_setglobal(this->mLua, name);
	}

	void ClassProxyHelper::BeginNewTable()
	{
		lua_getglobal(this->mLua, this->mName.c_str());
		if (lua_isnil(this->mLua, -1))
		{
			lua_newtable(this->mLua);
			{
				lua_pushstring(this->mLua, "__index");
				lua_pushcclosure(this->mLua, ClassMateProxy::Index, 0);
				lua_rawset(this->mLua, -3);
			}
			lua_setglobal(this->mLua, this->mName.c_str());
		}
	}

	void ClassProxyHelper::BeginNewTable(const std::string & name)
	{
		this->mName = name;
		this->BeginNewTable();		
	}

	ClassProxyHelper ClassProxyHelper::Clone(const std::string& name)
	{
		return ClassProxyHelper(this->mLua, name);
	}
}
