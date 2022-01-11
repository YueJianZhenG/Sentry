﻿#include"LuaScriptComponent.h"
#include<Script/luadebug.h>
#include<Script/luaExtension.h>
#include<Script/SystemExtension.h>
#include<Script/CoroutineExtension.h>

#include <Core/App.h>
#include<Util/DirectoryHelper.h>
#include<Util/FileHelper.h>
#include<Util/MD5.h>
#include"Async/LuaTaskSource.h"
#include <Component/ServiceBase/LuaServiceComponent.h>
namespace GameKeeper
{
    bool LuaScriptComponent::Awake()
    {
        this->mLuaEnv = luaL_newstate();
        luaL_openlibs(mLuaEnv);

        this->PushClassToLua();
        this->RegisterExtension();
        return this->LoadAllFile();
    }

    bool LuaScriptComponent::LateAwake()
    {

        this->OnPushGlobalObject();
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
        std::vector<std::string> luaPaths;
        std::vector<std::string> luaFiles;
		const ServerConfig & config = App::Get().GetConfig();
		LOG_CHECK_RET_FALSE(config.GetValue("ScriptPath", luaPaths));
        const std::string & workPath = App::Get().GetServerPath().GetWorkPath();
        for(const std::string & path : luaPaths)
        {
            std::string fullPath = workPath + path;
            if(!Helper::Directory::GetFilePaths(fullPath, "*.lua",luaFiles))
            {
                LOG_ERROR("load", path, "lua file failure");
                return false;
            }
        }

		std::string dir, name, luaFile;
		for (std::string & path : luaFiles)
		{
#ifdef __DEBUG__
            LOG_INFO("load lua file " , path);
#endif
			if (Helper::File::ReadTxtFile(path, luaFile)
				&& Helper::Directory::GetDirAndFileName(path, dir, name))
			{
                const std::string & md5 = Helper::Md5::GetMd5(luaFile);
				auto iter = this->mLuaFileMd5s.find(name);
				if (iter == this->mLuaFileMd5s.end())
				{
					mLuaFileMd5s.emplace(name, md5);
					LOG_CHECK_RET_FALSE(this->LoadLuaScript(path));
				}
				else if(iter->second != md5)
				{
                    mLuaFileMd5s[name] = md5;
                    LOG_CHECK_RET_FALSE(this->LoadLuaScript(path));
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

    bool LuaScriptComponent::GetLuaTable(const std::string &name)
    {
        auto iter = this->mGlobalRefMap.find(name);
        if(iter != this->mGlobalRefMap.end())
        {
            int ref = iter->second;
            lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, ref);
            return (bool)lua_istable(this->mLuaEnv, -1);
        }
        lua_getglobal(this->mLuaEnv, name.c_str());
        if(lua_istable(this->mLuaEnv, -1))
        {
            int ref = luaL_ref(this->mLuaEnv, LUA_REGISTRYINDEX);
            this->mGlobalRefMap.emplace(name, ref);
            return true;
        }
        return false;
    }

    bool LuaScriptComponent::GetLuaFunction(const std::string &tab, const std::string &func)
    {
        if(!this->GetLuaTable(tab))
        {
            return false;
        }
        lua_getfield(this->mLuaEnv, -1, func.c_str());
        if(lua_isfunction(this->mLuaEnv, -1))
        {
            int ref = luaL_ref(this->mLuaEnv, LUA_REGISTRYINDEX);
            const std::string name = fmt::format("{0}.{1}", tab, func);
            this->mGlobalRefMap.emplace(name, ref);
            return true;
        }
        return false;
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
        LOG_ERROR("load", filePath, "failure : ", lua_tostring(mLuaEnv, -1));
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
		if (Helper::Directory::GetFilePaths(path, "*.lua",luaFiles))
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
        ClassProxyHelper::BeginRegister<LuaTaskSource>(this->mLuaEnv, "LuaTaskSource");
        ClassProxyHelper::PushCtor<LuaTaskSource>(this->mLuaEnv);
        ClassProxyHelper::PushMemberFunction(this->mLuaEnv, "SetResult", &LuaTaskSource::SetResult);

        ClassProxyHelper::BeginRegister<App>(this->mLuaEnv, "App");
        ClassProxyHelper::PushMemberFunction<App>(this->mLuaEnv, "GetDelaTime", &App::GetDelaTime);     

        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "Helper::Time", "GetDateStr", Helper::Time::GetDateStr);
        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "Helper::Time", "GetDateString", Helper::Time::GetDateString);
        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "Helper::Time", "GetSecTimeStamp", Helper::Time::GetSecTimeStamp);
        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "Helper::Time", "GetMilTimestamp", Helper::Time::GetMilTimestamp);
        //ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "Helper::Time", "GetMicTimeStamp", Helper::Time::GetMicTimeStamp);
        ClassProxyHelper::PushStaticFunction(this->mLuaEnv, "Helper::Time", "GetYearMonthDayString",
                                             Helper::Time::GetYearMonthDayString);

        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "Call", SystemExtension::Call);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "Sleep", SystemExtension::Sleep);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "AsyncWait", SystemExtension::AddTimer);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "RemoveTimer", SystemExtension::RemoveTimer);

        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "GameKeeper", "GetManager", SystemExtension::GetManager);

        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "Log", "Debug", LuaAPIExtension::DebugLog);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "Log","Warning",LuaAPIExtension::DebugWarning);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv,"Log","Error",LuaAPIExtension::DebugError);
        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv,"Log","Info", LuaAPIExtension::DebugInfo);

        ClassProxyHelper::PushStaticExtensionFunction(this->mLuaEnv, "coroutine", "sleep", CoroutineExtension::Sleep);
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