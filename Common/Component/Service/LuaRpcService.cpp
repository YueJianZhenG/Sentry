﻿#include"LuaRpcService.h"
#include"Script/LuaTable.h"
#include"Method/LuaServiceMethod.h"
#include"Component/Lua/LuaScriptComponent.h"
#include"Component/Rpc//RpcConfigComponent.h"

namespace Sentry
{
	LuaRpcService::LuaRpcService()
		: mLuaEnv(nullptr)
	{

	}

	LuaRpcService::~LuaRpcService()
	{
		//luaL_unref(this->mLuaEnv, LUA_REGISTRYINDEX, this->mIdx);
	}

	bool LuaRpcService::Awake()
	{
		return true;
	}

	void LuaRpcService::OnHotFix()
	{

	}

	bool LuaRpcService::LateAwake()
	{
		this->mLuaComponent = this->GetComponent<LuaScriptComponent>();
		this->mConfigComponent = this->GetComponent<RpcConfigComponent>();
		LOG_CHECK_RET_FALSE(this->mLuaComponent && this->mConfigComponent);
		LOG_CHECK_RET_FALSE(this->mLuaEnv = this->mLuaComponent->GetLuaEnv());
		std::shared_ptr<Lua::LuaTable> luaTable = Lua::LuaTable::Create(this->mLuaEnv, this->GetName());
		if (luaTable == nullptr)
		{
			LOG_ERROR(this->GetName(), " is not lua table");
			return false;
		}
		std::shared_ptr<Lua::LuaFunction> awakeFunction = luaTable->GetFunction("Awake");
		std::shared_ptr<Lua::LuaFunction> lateAwakeFunction = luaTable->GetFunction("LateAwake");

		if (awakeFunction != nullptr)
		{
			LOG_CHECK_RET_FALSE(awakeFunction->Func<bool>());
		}
		if (lateAwakeFunction != nullptr)
		{
			LOG_CHECK_RET_FALSE(lateAwakeFunction->Func<bool>());
		}

		std::vector<std::string> methods;
		this->mConfigComponent->GetMethods(this->GetName(), methods);

		for (const std::string& method : methods)
		{
			int ref = luaTable->GetRef();
			lua_rawgeti(this->mLuaEnv, LUA_REGISTRYINDEX, ref);
			lua_getfield(this->mLuaEnv, -1, method.c_str());
			LOG_CHECK_RET_FALSE(lua_isfunction(this->mLuaEnv, -1));
			int idx = luaL_ref(this->mLuaEnv, LUA_REGISTRYINDEX);
			auto config = this->mConfigComponent
				->GetProtocolConfig(this->GetName() + "." + method);
			LOG_INFO("add new lua service method : {0}.{1}", this->GetName(), method);
			this->AddMethod(std::make_shared<LuaServiceMethod>(config, this->mLuaEnv, idx));
		}
		return true;
	}

	void LuaRpcService::OnStart()
	{
		const char * tab = this->GetName().c_str();
		LuaTaskSource * luaTaskSource = this->mLuaComponent->Call(tab, "OnStart");
		if(luaTaskSource != nullptr && luaTaskSource->Await<bool>())
		{
			LOG_WARN("Invoke ", this->GetName(), " Start Coroutine")
		}
	}
}