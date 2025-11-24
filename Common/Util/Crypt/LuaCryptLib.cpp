//
// Created by zmhy0073 on 2022/9/21.
//


#include "LuaCryptLib.h"
#include "Util/Crypt/sha1.h"
#include "Proto/Bson/base64.h"
#include "Util/Crypt/MD5Helper.h"
namespace lua
{
	int md5::Hex(lua_State* L)
	{
		size_t size = 0;
		const char * str = luaL_checklstring(L, 1, &size);
		std::string output = help::md5::GetHex(str, size);
		lua_pushlstring(L, output.c_str(), output.size());
		return 1;
	}

	int md5::File(lua_State* L)
	{
		size_t size = 0;
		static std::string path;
		static std::string output;
		const char * str = luaL_checklstring(L, 1, &size);

		output.clear();
		path.assign(str, size);
		if(!help::md5::FileMd5(path, output))
		{
			return 0;
		}
		lua_pushlstring(L, output.c_str(), output.size());
		return 1;
	}

    int md5::ToString(lua_State * L)
    {
		size_t size = 0;
		const char * str = luaL_checklstring(L, 1, &size);
		std::string output = help::md5::GetMd5(str, size);
        lua_pushlstring(L, output.c_str(), output.size());
        return 1;
    }
}

namespace lua
{
	int base64::Encode(lua_State* l)
	{
		size_t size = 0;
		const char * str = luaL_checklstring(l, 1, &size);
		const std::string target = _bson::base64::encode(str, (int)size);
		lua_pushlstring(l, target.c_str(), target.size());
		return 1;
	}

	int base64::Decode(lua_State* l)
	{
		size_t size = 0;
		const char * str = luaL_checklstring(l, 1, &size);
		const std::string target = _bson::base64::decode(std::string(str, size));
		lua_pushlstring(l, target.c_str(), target.size());
		return 1;
	}
}

namespace lua
{
	int sha1::Xor(lua_State* L)
	{
		size_t size1, size2 = 0;
		const char * str1 = luaL_checklstring(L, 1, &size1);
		const char * str2 = luaL_checklstring(L, 2, &size2);
		std::string output = help::Sha1::XorString((std::string(str1, size1)), std::string(str2, size2));
		lua_pushlstring(L, output.c_str(), output.size());
		return 1;
	}
	int sha1::Hash(lua_State* L)
	{
		size_t size = 0;
		const char * str = luaL_checklstring(L, 1, &size);
		std::string output = help::Sha1::GetHash(str, size);
		lua_pushlstring(L, output.c_str(), output.size());
		return 1;
	}

	int sha1::HMac(lua_State* L)
	{
		size_t size1, size2 = 0;
		const char * str1 = luaL_checklstring(L, 1, &size1);
		const char * str2 = luaL_checklstring(L, 2, &size2);
		{
			std::string key(str1, size1);
			std::string text(str2, size2);
			std::string output = help::Sha1::GetHMacHash(key, text);
			lua_pushlstring(L, output.c_str(), output.size());
		}
		return 1;
	}
}
