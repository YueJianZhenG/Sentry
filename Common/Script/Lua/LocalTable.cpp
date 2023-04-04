//
// Created by zmhy0073 on 2022/8/15.
//

#include"LocalTable.h"
#include"Util/Md5/MD5.h"
#include"Util/File/DirectoryHelper.h"

namespace Lua
{
    LocalTable::LocalTable(lua_State *lua)
    {
        this->mIndex = 0;
        this->mLua = lua;
    }

    LocalTable::~LocalTable()
    {
        this->Clear();
    }

    void LocalTable::Clear()
    {
        if(this->mIndex != 0)
        {
            lua_unref(this->mLua, this->mIndex);
            this->mIndex = 0;
        }
        this->mIndex = lua_ref(this->mLua);
        auto iter = this->mIndexMap.begin();
        for (; iter != this->mIndexMap.end(); iter++)
        {
            lua_unref(this->mLua, iter->second);
        }
        this->mIndexMap.clear();
    }

    bool LocalTable::GetFunction(const std::string &func, bool ref)
    {
        auto iter = this->mIndexMap.find(func);
        if(iter != this->mIndexMap.end())
        {
            lua_getref(this->mLua, iter->second);
            return lua_isfunction(this->mLua, -1);
        }
        lua_getref(this->mLua, this->mIndex);
        if(lua_getfunction(this->mLua, -1, func.c_str()))
        {
            if(ref)
            {
                int index = lua_ref(this->mLua);
                lua_getref(this->mLua, index);
                this->mIndexMap.emplace(func, index);
            }
            return true;
        }
        return false;
    }

    bool LocalTable::Load(const std::string &path)
    {
        std::ifstream is(path);
        if (!is.is_open())
        {
            return false;
        }
        this->mPath = path;
        if (!Helper::Directory::GetFileName(path, this->mTableName))
        {
            return false;
        }
        size_t pos = this->mTableName.find('.');
        if(pos == std::string::npos)
        {
            return false;
        }
        MD5 md5(is);
        std::string str = md5.toString();
        if (str == this->mMd5)
        {
            return true;
        }
        if(!this->mTableName.empty())
        {
            lua_getglobal(this->mLua, this->mTableName.c_str());
            if(lua_istable(this->mLua, -1))
            {
                lua_pushnil(this->mLua);
                lua_setglobal(this->mLua, this->mTableName.c_str());
            }
        }

        if (luaL_dofile(this->mLua, path.c_str()) == LUA_OK)
        {
            this->mTableName = this->mTableName.substr(0, pos);
            lua_getglobal(this->mLua, this->mTableName.c_str());
            if (lua_istable(this->mLua, -1))
            {
                this->Clear();
                this->mMd5 = str;
                this->mIndex = lua_ref(this->mLua);
                return true;
            }
            return false;
        }
        return false;
    }
}