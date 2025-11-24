//
// Created by 64658 on 2025/1/16.
//

#ifdef __ENABLE_OPEN_SSL__
#include "LuaAes.h"
#include "Auth/Aes/Aes.h"
namespace lua
{
	int laes::Encode(lua_State* L)
	{
		size_t size1, size2 = 0;
		const char * key = luaL_checklstring(L, 1, &size1);
		const char * value = luaL_checklstring(L, 2, &size2);

		std::string output;
		if(!aes::Encode(std::string(key, size1), std::string(value, size2), output))
		{
			return 0;
		}
		lua_pushlstring(L, output.c_str(), output.size());
		return 1;
	}

	int laes::Decode(lua_State* L)
	{
		size_t size1, size2 = 0;
		const char * key = luaL_checklstring(L, 1, &size1);
		const char * value = luaL_checklstring(L, 2, &size2);

		std::string output;
		if(!aes::Decode(std::string(key, size1), std::string(value, size2), output))
		{
			return 0;
		}
		lua_pushlstring(L, output.c_str(), output.size());
		return 1;
	}
}
#endif