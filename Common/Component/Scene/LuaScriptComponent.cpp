﻿#include"LuaScriptComponent.h"
#include<Script/luadebug.h>
#include<Script/luaExtension.h>
#include<Script/SystemExtension.h>
#include<Script/CoroutineExtension.h>

#include <Core/App.h>
#include<Util/DirectoryHelper.h>
#include<Util/FileHelper.h>
#include<Util/MD5.h>

#include <Service/LuaServiceComponent.h>
namespace GameKeeper
{
    bool LuaScriptComponent::Awake()
    {
        this->mLuaEnv = luaL_newstate();
        luaL_openlibs(mLuaEnv);
        return true;
    }

    bool LuaScriptComponent::LateAwake()
    {
        this->PushClassToLua();
        this->RegisterExtension();
        this->OnPushGlobalObject();
        LOG_CHECK_RET_FALSE(this->LoadAllFile());

        if (lua_getfunction(this->mLuaEnv, "Main", "Awake"))
        {
            if (lua_pcall(this->mLuaEnv, 0, 0, 0) != 0)
            {
                LOG_ERROR(lua_tostring(this->mLuaEnv, -1));
                return false;
            }
            return (bool) lua_toboolean(this->mLuaEnv, -1);
        }
        if (!lua_getfunction(this->mLuaEnv, "Main", "Start"))
        {
            return false;
        }
        lua_State *coroutine = lua_newthread(this->mLuaEnv);
        lua_pushvalue(this->mLuaEnv, -2);
        lua_xmove(this->mLuaEnv, coroutine, 1);
        lua_presume(coroutine, this->mLuaEnv, 0);
        return true;
    }

	bool LuaScriptComponent::LoadAllFile()
	{
		std::string luaDir;
		const ServerConfig & config = App::Get().GetConfig();
		LOG_CHECK_RET_FALSE(config.GetValue("ScriptPath", luaDir));

		std::vector<std::string> luaFiles;
		DirectoryHelper::GetFilePaths(luaDir, "*.lua",luaFiles);

		std::string dir, name, luaFile;
		for (std::string & path : luaFiles)
		{
			if (FileHelper::ReadTxtFile(path, luaFile)
				&& DirectoryHelper::GetDirAndFileName(path, dir, name))
			{
				MD5 md5(luaFile.c_str(), luaFile.size());
				auto iter = this->mLuaFileMd5s.find(name);
				if (iter == this->mLuaFileMd5s.end())
				{
					mLuaFileMd5s.emplace(name, md5.toString());
					LOG_CHECK_RET_FALSE(this->LoadLuaScript(path));
				}
				else
				{
					const std::string & oldMd5 = iter->second;
					const std::string & newMd5 = md5.toString();
					if (oldMd5 != newMd5)
					{
						mLuaFileMd5s[name] = newMd5;
						LOG_CHECK_RET_FALSE(this->LoadLuaScript(path));
					}
				}
			}
		}
        return true;
	}
	
    void LuaScriptComponent::OnDestory()
    {
        if (this->mLuaEnv != nullptr)
        {
            lua_close(this->mLuaEnv);
        }
    }

	int LuaScriptComponent::GetLuaRef(const std::string & tab, const std::string & field)
	{
		const std::string key = tab + "." + field;
		auto iter = this->mGlobalRefMap.find(key);
		if (iter != this->mGlobalRefMap.end())
		{
			return iter->second;
		}
		lua_getglobal(this->mLuaEnv, tab.c_str());
		if (!lua_istable(this->mLuaEnv, -1))
		{
			LOG_ERROR("find lua object fail " << tab);
			return 0;
		}
		lua_getfield(this->mLuaEnv, -1, field.c_str());
		if (lua_isnil(this->mLuaEnv, -1))
		{
			LOG_ERROR("find lua object field fail " << field);
			return 0;
		}
		int ref = luaL_ref(this->mLuaEnv, LUA_REGISTRYINDEX);
		this->mGlobalRefMap.emplace(key, ref);
		return ref;
	}

	int LuaScriptComponent::GetLuaRef(const std::string &name)
	{
		auto iter = this->mGlobalRefMap.find(name);
		if (iter != this->mGlobalRefMap.end())
		{
			return iter->second;
		}
		lua_getglobal(this->mLuaEnv, name.c_str());
		if (lua_isnil(this->mLuaEnv, -1))
		{
			LOG_ERROR("find lua object field fail " << name);
			return 0;
		}
		int ref = luaL_ref(this->mLuaEnv, LUA_REGISTRYINDEX);
		this->mGlobalRefMap.emplace(name, ref);
		return ref;
	}


    bool LuaScriptComponent::LoadLuaScript(const std::string filePath)
    {
        lua_pushcclosure(mLuaEnv, LuaDebug::onError, 0);
        int errfunc = lua_gettop(mLuaEnv);
        if (luaL_loadfile(mLuaEnv, filePath.c_str()) == 0)
        {
            lua_pcall(mLuaEnv, 0, 1, errfunc);
            //LOG_DEBUG("load lua script success path :" << filePath);
            lua_pop(mLuaEnv, 2);
			return true;
        }
        LOG_ERROR("load " << filePath << " failure : " << lua_tostring(mLuaEnv, -1));
        lua_pop(mLuaEnv, 1);
        return false;
    }

    void LuaScriptComponent::ClearRequirePath()
    {
        std::string path = "";
        lua_getglobal(mLuaEnv, "package");
        lua_pushlstring(mLuaEnv, path.c_str(), path.size());
        lua_setfield(mLuaEnv, -3, "path");
    }

    void LuaScriptComponent::AddRequirePath(const std::string path)
    {
		std::vector<std::string> luaFiles;
		if (DirectoryHelper::GetFilePaths(path, "*.lua",luaFiles))
		{
			for (std::string & file : luaFiles)
			{
				this->LoadLuaScript(file);
			}
		}
        lua_getglobal(mLuaEnv, "package");
        lua_getfield(mLuaEnv, -1, "path");
        std::string nRequestPath = lua_tostring(mLuaEnv, -1);
        if (nRequestPath.find(path) != std::string::npos)
        {
            return;
        }
        char pathBuffer[4096] = {0};
#ifdef _MSC_VER
        size_t size = sprintf_s(pathBuffer, "%s;%s/?.lua", nRequestPath.c_str(), path.c_str());
#else
        size_t size = sprintf(pathBuffer, "%s;%s/?.lua", nRequestPath.c_str(), path.c_str());
#endif
        lua_pushlstring(mLuaEnv, pathBuffer, size);
        lua_setfield(mLuaEnv, -3, "path");
    }

    void LuaScriptComponent::PushClassToLua()
    {
        ClassProxyHelper::BeginRegister<App>(this->mLuaEnv, "App");
        ClassProxyHelper::PushMemberFunction<App>(this->mLuaEnv, "GetDelaTime", &App::GetDelaTime);     

        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "TimeHelper", "GetDateStr", TimeHelper::GetDateStr);
        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "TimeHelper", "GetDateString", TimeHelper::GetDateString);
        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "TimeHelper", "GetSecTimeStamp", TimeHelper::GetSecTimeStamp);
        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "TimeHelper", "GetMilTimestamp", TimeHelper::GetMilTimestamp);
        //ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "TimeHelper", "GetMicTimeStamp", TimeHelper::GetMicTimeStamp);
        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "TimeHelper", "GetYearMonthDayString",
                                             TimeHelper::GetYearMonthDayString);

        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "Call", SystemExtension::Call);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "WaitForSleep", SystemExtension::Sleep);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "AsyncWait", SystemExtension::AddTimer);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "RemoveTimer", SystemExtension::RemoveTimer);

        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "GetManager", SystemExtension::GetManager);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "Debug", LuaAPIExtension::DebugLog);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper","Warning",LuaAPIExtension::DebugWarning);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv,"GameKeeper","Error",LuaAPIExtension::DebugError);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv,"GameKeeper","Info", LuaAPIExtension::DebugInfo);
    }

    void LuaScriptComponent::OnPushGlobalObject()
    {

    }

    void LuaScriptComponent::RegisterExtension()
    {
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "Log", LuaAPIExtension::DebugLog);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "Info", LuaAPIExtension::DebugInfo);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "Error", LuaAPIExtension::DebugError);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "Warning", LuaAPIExtension::DebugWarning);


        lua_getglobal(this->mLuaEnv, "coroutine");
        lua_pushtablefunction(this->mLuaEnv, "sleep", CoroutineExtension::Sleep);
        lua_pushtablefunction(this->mLuaEnv, "start", CoroutineExtension::Start);


    }
}