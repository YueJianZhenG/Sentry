//
// Created by leyi on 2023/6/29.
//

#include "LuaFile.h"
#include "Util/Tools/String.h"
#include "Util/File/FileHelper.h"
#include "Util/File/DirectoryHelper.h"
namespace lua
{
	int LuaFile::Write(lua_State * L)
	{
		size_t size = 0;
		size_t count = 0;
		const char* str1 = luaL_checklstring(L, 1, &count);
		const char* content = luaL_checklstring(L, 2, &size);

		std::ofstream ofs;
		std::string director;
		std::string path(str1, count);
		help::dir::GetDirByPath(path, director);
#ifdef __OS_WIN__
		director = help::text::Utf8ToGB2312(director);
#endif
		if(!help::dir::DirectorIsExist(director))
		{
			help::dir::MakeDir(director);
		}
		ofs.open(path, std::ios::ate | std::ios::binary | std::ios::out);
		if (!ofs.is_open())
		{
#ifdef __OS_WIN__
			std::string str = help::text::Utf8ToGB2312(path);
			ofs.open(str, std::ios::ate | std::ios::binary | std::ios::out);
			if(!ofs.is_open())
			{
				return 0;
			}
#else
			return 0;
#endif
		}
		ofs.write(content, size);
		ofs.flush();
		ofs.close();
		lua_pushinteger(L, size);
		return 1;
	}

	int LuaFile::GetFileName(lua_State* lua)
	{
		size_t count = 0;
		static std::string path;
		const char * str = luaL_checklstring(lua, 1, &count);

		path.assign(str, count);
		size_t pos = path.find_last_of('/');
		if(pos == std::string::npos)
		{
			lua_pushstring(lua, path.c_str());
			return 1;
		}
		std::string fileName = path.substr(pos + 1);
		lua_pushlstring(lua, fileName.c_str(), fileName.size());
		return 1;
	}

	int LuaFile::GetFileSize(lua_State * lua)
	{
		size_t count = 0;
		static std::string path;
		const char * str = luaL_checklstring(lua, 1, &count);

		size_t fileSize = 0;
		path.assign(str, count);
		help::fs::GetFileSize(path, fileSize);

		lua_pushinteger(lua, fileSize);
		return 1;
	}

	int LuaFile::Read(lua_State* L)
	{
		size_t count1;
		std::string content;
		static std::string path;
		const char* str1 = luaL_checklstring(L, 1, &count1);
		{
			path.assign(str1, count1);
		}
		if(!help::fs::ReadTxtFile(path, content))
		{
			return 0;
		}
		lua_pushlstring(L, content.c_str(), content.size());
		return 1;
	}

	int LuaFile::Remove(lua_State * L)
	{
		const char* path = luaL_checkstring(L, 1);
		if(!help::fs::FileIsExist(path))
		{
			return 0;
		}
		int result = std::remove(path);
		lua_pushinteger(L, result);
		return 1;
	}

	int LuaFile::Find(lua_State* L)
	{
		size_t count1, count2 = 0;
		const char* str1 = luaL_checklstring(L, 1, &count1);
		const char* str2 = luaL_checklstring(L, 2, &count2);

		std::vector<std::string> files;
		std::string path(str1, count1);
		std::string type(str2, count2);
		if (!help::dir::GetFilePaths(path, type, files))
		{
			return 0;
		}
		lua_newtable(L);
		for (size_t index = 0; index < files.size(); index++)
		{
			lua_pushstring(L, files[index].c_str());
			lua_seti(L, -2, index + 1);
		}
		return 1;
	}

	int LuaFile::IsExist(lua_State* L)
	{
		const char * path = luaL_checkstring(L, 1);
		lua_pushboolean(L, help::fs::FileIsExist(path));
		return 1;
	}

	int LuaFile::GetFiles(lua_State* L)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 1, &count);

		std::string dir(str, count);
		std::vector<std::string> files;
		if(lua_isstring(L, 2))
		{
			std::string type = lua_tostring(L, 2);
			if(!help::dir::GetFilePaths(dir, type, files))
			{
				return 0;
			}
		}
		else
		{
			if(!help::dir::GetFilePaths(dir, files))
			{
				return 0;
			}
		}

		lua_createtable(L, 0, files.size());
		for(size_t index = 0; index < files.size(); index++)
		{
			std::string & path = files[index];
			lua_pushstring(L, path.c_str());
			lua_seti(L, -2, index + 1);
		}
		return 1;
	}

	int LuaFile::GetLastWriteTime(lua_State* L)
	{
		const char * path = luaL_checkstring(L, 1);
		long long time = help::fs::GetLastWriteTime(path);
		lua_pushinteger(L, time);
		return 1;
	}

	int LuaDir::Make(lua_State* lua)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(lua, 1, &count);

		std::string dir(str, count);
		if(help::dir::DirectorIsExist(dir))
		{
			lua_pushboolean(lua, true);
			return 1;
		}
		lua_pushboolean(lua, help::dir::MakeDir(dir));
		return 1;
	}

	int LuaDir::IsExist(lua_State* lua)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(lua, 1, &count);

		std::string dir(str, count);
		lua_pushboolean(lua, help::dir::DirectorIsExist(dir));
		return 1;
	}
}
