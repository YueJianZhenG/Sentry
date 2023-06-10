﻿#include "LuaComponent.h"

#include "Entity/Actor/App.h"
#include "Util/Md5/MD5.h"
#include "Util/Guid/Guid.h"
#include "Util/File/FileHelper.h"
#include "Util/File/DirectoryHelper.h"
#include "Async/Lua/WaitLuaTaskSource.h"
#include "Rpc//Lua/LuaServiceTaskSource.h"
#include "Log/Lua/LuaLogger.h"
#include "Util/Json/Lua/Json.h"
#include "Async/Lua/LuaCoroutine.h"
#include "Util/Md5/LuaMd5.h"
#include "Core/System/System.h"
#include "Util/Json/JsonWriter.h"
#include "Lua/Engine/Function.h"
#include "Lua/Module/LuaModule.h"
#include "Entity/Lua/LuaActor.h"
#include "Lua/Engine/ClassProxyHelper.h"
#include "Server/Component/ConfigComponent.h"
using namespace Lua;
namespace Tendo
{
	LuaComponent::LuaComponent()
	{
		this->mLuaEnv = nullptr;
	}

	bool LuaComponent::Awake()
	{
		this->mMainModule = nullptr;
		const rapidjson::Value * config = this->mApp->Config()->GetValue("lua");
		LOG_CHECK_RET_FALSE(config != nullptr && this->mLuaConfig->Init(*config));

		this->mLuaEnv = luaL_newstate();
		luaL_openlibs(mLuaEnv);
		return true;
	}

	bool LuaComponent::LateAwake()
	{
		{
			Lua::ClassProxyHelper os(this->mLuaEnv, "os");

			os.PushMember("dir", System::WorkPath());
			os.PushStaticFunction("ms", Helper::Time::NowMilTime);
			os.PushStaticFunction("time", Helper::Time::NowSecTime);
#ifdef __OS_MAC__
			os.PushMember("platform", std::string("mac"));
#elif __OS_LINUX__
			os.PushMember("platform", std::string("linux"));
#elif __OS__WIN__
			os.PushMember("platform", std::string("win"));
#endif

#ifdef __DEBUG__
			os.PushMember("debug", true);
#else
			os.PushMember("debug", false);
#endif
		}
		{
			Lua::ClassProxyHelper app(this->mLuaEnv, "App");
			app.BeginRegister<App>();
			app.PushExtensionFunction("Send", LuaActor::Send);
			app.PushExtensionFunction("Call", LuaActor::Call);
			app.PushExtensionFunction("Random", LuaActor::Random);
			app.PushExtensionFunction("GetListen", LuaActor::GetListen);
			app.PushExtensionFunction("SendToClient", LuaActor::SendToClient);
		}

		Lua::ClassProxyHelper luaRegister1(this->mLuaEnv, "WaitLuaTaskSource");
		luaRegister1.BeginRegister<WaitLuaTaskSource>();
		luaRegister1.PushCtor<WaitLuaTaskSource>();
		luaRegister1.PushStaticExtensionFunction("SetResult", WaitLuaTaskSource::SetResult);

		Lua::ClassProxyHelper luaRegister2(this->mLuaEnv, "LuaServiceTaskSource");
		luaRegister2.BeginRegister<LuaServiceTaskSource>();
		luaRegister2.PushExtensionFunction("SetRpc", LuaServiceTaskSource::SetRpc);
		luaRegister2.PushExtensionFunction("SetHttp", LuaServiceTaskSource::SetHttp);

		Lua::ClassProxyHelper luaRegister3(this->mLuaEnv, "Time");
		luaRegister3.BeginNewTable();
		luaRegister3.PushStaticFunction("GetDateStr", Helper::Time::GetDateStr);
		luaRegister3.PushStaticFunction("GetDateString", Helper::Time::GetDateString);
		luaRegister3.PushStaticFunction("NowSecTime", Helper::Time::NowSecTime);
		luaRegister3.PushStaticFunction("NowMilTime", Helper::Time::NowMilTime);
		luaRegister3.PushStaticFunction("GetYearMonthDayString", Helper::Time::GetYearMonthDayString);

		Lua::ClassProxyHelper luaRegister4(this->mLuaEnv, "coroutine");
		luaRegister4.PushExtensionFunction("start", Lua::Coroutine::Start);
		luaRegister4.PushExtensionFunction("sleep", Lua::Coroutine::Sleep);

		Lua::ClassProxyHelper luaRegister5(this->mLuaEnv, "Logger");
		luaRegister5.BeginNewTable();
		luaRegister5.PushExtensionFunction("Output", Lua::Log::Output);

		Lua::ClassProxyHelper luaRegister55(this->mLuaEnv, "ConsoleLog");
		luaRegister55.BeginNewTable();
		luaRegister55.PushExtensionFunction("Show", Lua::Console::Show);

		Lua::ClassProxyHelper luaRegister6(this->mLuaEnv, "Guid");
		luaRegister6.BeginNewTable();
		luaRegister6.PushStaticFunction("Create", Helper::Guid::Create);

		Lua::ClassProxyHelper luaRegister7(this->mLuaEnv, "rapidjson");
		luaRegister7.BeginNewTable();
		luaRegister7.PushExtensionFunction("encode", Lua::RapidJson::Encode);
		luaRegister7.PushExtensionFunction("decode", Lua::RapidJson::Decode);

		Lua::ClassProxyHelper luaRegister8(this->mLuaEnv, "Md5");
		luaRegister8.BeginNewTable();
		luaRegister8.PushExtensionFunction("ToString", Lua::Md5::ToString);

		std::vector<ILuaRegister*> components;
		this->mApp->GetComponents(components);
		for (ILuaRegister* component: components)
		{
			const std::string& name = dynamic_cast<Component*>(component)->GetName();
			Lua::ClassProxyHelper luaRegister(this->mLuaEnv, name);
			component->OnLuaRegister(luaRegister);
		}
		return this->LoadAllFile();
	}

	bool LuaComponent::GetFunction(const std::string& tab, const std::string& func)
	{
		return Lua::Function::Get(this->mLuaEnv, tab.c_str(), func.c_str());
	}

	Lua::LuaModule* LuaComponent::LoadModule(const std::string& name)
	{
		for(const std::string & path : this->mLuaConfig->Requires())
		{
			std::string fileName;
			std::vector<std::string> luaFiles;
			Helper::Directory::GetFilePaths(path, "*.lua", luaFiles);
			for (const std::string& filePath : luaFiles)
			{
				Helper::File::GetFileName(filePath, fileName);
				if (name == fileName)
				{
					return this->LoadModuleByPath(filePath);
				}
			}
		}
		return nullptr;
	}

	Lua::LuaModule* LuaComponent::LoadModuleByPath(const std::string& path)
	{
		std::string name;
		Helper::File::GetFileName(path, name);
		LuaModule* luaModule = new LuaModule(this->mLuaEnv, name);
		if (!luaModule->LoadFromPath(path))
		{
			delete luaModule;
			LOG_ERROR("load lua module [" << name << "] error");
			return nullptr;
		}
		LOG_INFO("start load lua module [" << name << "]");
		return luaModule;
	}


	void LuaComponent::OnSecondUpdate(const int tick)
	{
		if(this->mMainModule != nullptr)
		{
			this->mMainModule->Update(tick);
		}
	}

	void LuaComponent::Start()
	{
		if(this->mMainModule != nullptr)
		{
			this->mMainModule->Await("OnStart");
		}
	}

	void LuaComponent::Complete()
	{
		if(this->mMainModule != nullptr)
		{
			this->mMainModule->Await("OnComplete");
		}
	}

	bool LuaComponent::LoadAllFile()
	{
		for(const std::string & path : this->mLuaConfig->Requires())
		{
			this->AddRequire(path);
		}
		for(const std::string & path : this->mLuaConfig->LoadFiles())
		{
			if(luaL_dofile(this->mLuaEnv, path.c_str()) != LUA_OK)
			{
				LOG_FATAL(lua_tostring(this->mLuaEnv, -1));
				return false;
			}
		}

		if(this->mLuaConfig->Main().empty())
		{
			return true;
		}
		std::string main = this->mLuaConfig->Main();
		this->mMainModule = this->LoadModuleByPath(main);
		LOG_CHECK_RET_FALSE(this->mMainModule != nullptr);
		return true;
	}

	void LuaComponent::OnHotFix()
	{
		if(this->mMainModule != nullptr)
		{
			this->mMainModule->Hotfix();
		}
	}

	void LuaComponent::OnDestroy()
	{
		if(this->mMainModule != nullptr)
		{
			this->mMainModule->Await("OnStop");
			delete this->mMainModule;
			this->mMainModule = nullptr;
		}
		if (this->mLuaEnv != nullptr)
		{
			lua_close(this->mLuaEnv);
			this->mLuaEnv = nullptr;
		}
	}

	void LuaComponent::AddRequire(const std::string& path)
	{
		if (!path.empty())
		{
			size_t size = 0;
			lua_getglobal(this->mLuaEnv, "package");
			lua_getfield(this->mLuaEnv, -1, "path");
			const char* str = lua_tolstring(this->mLuaEnv, -1, &size);
			std::string fullPath = std::string(str, size) + ";" + path + "/?.lua";
			lua_pushlstring(this->mLuaEnv, fullPath.c_str(), fullPath.size());
			lua_setfield(this->mLuaEnv, -3, "path");
		}
	}

	double LuaComponent::CollectGarbage()
	{
		double start = this->GetMemorySize();
		if (Lua::Function::Get(this->mLuaEnv, "collectgarbage"))
		{
			lua_pushstring(this->mLuaEnv, "collect");
			if (lua_pcall(this->mLuaEnv, 1, 1, 0) != LUA_OK)
			{
				LOG_ERROR(lua_tostring(this->mLuaEnv, -1));
				return 0;
			}
		}
		return start - this->GetMemorySize();
	}

	double LuaComponent::GetMemorySize()
	{
		if (Lua::Function::Get(this->mLuaEnv, "collectgarbage"))
		{
			lua_pushstring(this->mLuaEnv, "count");
			if (lua_pcall(this->mLuaEnv, 1, 1, 0) != LUA_OK)
			{
				LOG_ERROR(lua_tostring(this->mLuaEnv, -1));
				return 0;
			}
			return lua_tonumber(this->mLuaEnv, -1);
		}
		return 0;
	}

	void LuaComponent::OnRecord(Json::Writer& document)
	{
		double size = this->GetMemorySize();
		double free = this->CollectGarbage();
		document.Add("free").Add(fmt::format("{0}mb", free / 1024));
		document.Add("memory").Add(fmt::format("{0}mb", size / 1024));
	}
}