//
// Created by 64658 on 2025/9/13.
//

#include "LuaMath.h"
#include "Util/Tools/Random.h"
namespace lua
{
	int math::Random(lua_State* L)
	{
		if(lua_isinteger(L, 1))
		{
			long long min = luaL_checkinteger(L, 1);
			long long max = luaL_checkinteger(L, 2);
			long long value = help::Rand::Random<long long>(min, max);
			lua_pushinteger(L, value);
		}
		else if(lua_isnumber(L, 1))
		{
			double min = luaL_checknumber(L, 1);
			double max = luaL_checknumber(L, 2);
			double value = help::Rand::Random<double>(min, max);
			lua_pushnumber(L, value);
		}
		else
		{
			double min = -1;
			double max = 1;
			double value = help::Rand::Random<double>(min, max);
			lua_pushnumber(L, value);
		}
		return 1;
	}
}