//
// Created by zmhy0073 on 2022/9/21.
//

#ifndef APP_LUACRYPTLIB_H
#define APP_LUACRYPTLIB_H
#include"Lua/Lib/Lib.h"
namespace lua
{
    namespace md5
    {
		int Hex(lua_State *lua);
		int File(lua_State *lua);
        int ToString(lua_State *lua);
    }

	namespace base64
	{
		extern int Encode(lua_State * l);
		extern int Decode(lua_State * l);
	}

	namespace sha1
	{
		extern int Xor(lua_State * L);
		extern int Hash(lua_State * L);
		extern int HMac(lua_State * L);
	}
}

#endif //APP_LUACRYPTLIB_H
