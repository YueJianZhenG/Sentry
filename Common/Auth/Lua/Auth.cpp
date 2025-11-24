//
// Created by yy on 2024/1/1.
//

#include "Auth.h"
#include "Auth/Jwt/Jwt.h"
#include "Yyjson/Lua/ljson.h"
namespace lua
{
	int ljwt::Create(lua_State* L)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 1, &count);

		std::string data;
		std::string key(str, count);
		if (lua_isstring(L, 2))
		{
			data = lua_tostring(L, 2);
		}
		else if (lua_istable(L, 2))
		{
			if(!yyjson::read(L, 2, data))
			{
				return 0;
			}
		}
		std::string token = jwt::Encode(data, key);
		lua_pushlstring(L, token.c_str(), token.size());
		return 1;
	}

	int ljwt::Verify(lua_State* L)
	{
		size_t len1, len2 = 0;
		const char * str1 = luaL_checklstring(L, 1, &len1);
		const char * str2 = luaL_checklstring(L, 2, &len2);

		std::string data;
		std::string key(str2, len2);
		std::string token(str1, len1);
		if(!jwt::Decode(token, key, data))
		{
			return 0;
		}
		return yyjson::write(L, data.c_str(), data.size()) ? 1: 0;
	}
}
