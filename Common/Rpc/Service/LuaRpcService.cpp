﻿#include"LuaRpcService.h"
#include"Lua/Table.h"
#include"Lua/Function.h"
#include"Lua/LuaServiceMethod.h"
#include"Component/LuaScriptComponent.h"
#include"Config/ServiceConfig.h"
#include"Method/MethodRegister.h"
namespace Sentry
{
	LuaRpcService::LuaRpcService()
		: mLuaEnv(nullptr)
	{
        this->mWaitCount = 0;
        this->mIsHandlerMessage = false;
	}

	LuaRpcService::~LuaRpcService()
	{
		//luaL_unref(this->mLuaEnv, LUA_REGISTRYINDEX, this->mIdx);
	}

	bool LuaRpcService::Start()
	{
        std::string location;
        const ServerConfig * config = ServerConfig::Inst();
        LOG_CHECK_RET_FALSE(config->GetLocation("rpc", location));
		this->mMethodRegister = std::make_shared<ServiceMethodRegister>(this);
        const RpcServiceConfig * rpcServiceConfig = RpcConfig::Inst()->GetConfig(this->GetName());

		std::vector<const RpcMethodConfig *> rpcInterConfigs;
        LOG_CHECK_RET_FALSE(rpcServiceConfig->GetMethodConfigs(rpcInterConfigs) > 0);

		for(const RpcMethodConfig * rpcInterfaceConfig : rpcInterConfigs)
		{
			const char* func = rpcInterfaceConfig->Method.c_str();
			const char * tab = rpcInterfaceConfig->Service.c_str();
			if (!Lua::Function::Get(this->mLuaEnv, tab, func))
			{
				LOG_ERROR("not find rpc method = [" << tab << '.' << func << ']');
				return false;
			}
			if (!this->mMethodRegister->AddMethod(std::make_shared<LuaServiceMethod>
			        (rpcInterfaceConfig, this->mLuaEnv)))
			{
				return false;
			}
		}
        if(Lua::Table::Get(this->mLuaEnv, this->GetName()))
        {
            lua_pushboolean(this->mLuaEnv, true);
            lua_setfield(this->mLuaEnv, -2, "IsStartService");
        }
        const char * t = this->GetName().c_str();
        if(Lua::lua_getfunction(this->mLuaEnv, t, "OnStart"))
        {
            if(lua_pcall(this->mLuaEnv, 0, 0, 0) != LUA_OK)
            {
                LOG_ERROR(lua_tostring(this->mLuaEnv, -1));
                return false;
            }
        }
        this->mIsHandlerMessage = true;
        return true;
	}

	XCode LuaRpcService::Invoke(const std::string &name, std::shared_ptr<Rpc::Packet> message)
	{
		if(!this->IsStartService())
		{
			LOG_ERROR(this->GetName() << " is not start");
			return XCode::CallServiceNotFound;
		}
        if(!this->mIsHandlerMessage)
        {
            return XCode::CallServiceNotFound;
        }
		std::shared_ptr<ServiceMethod> serviceMethod = this->mMethodRegister->GetMethod(name);
		if (serviceMethod == nullptr)
		{
			return XCode::CallServiceNotFound;
		}
        this->mWaitCount++;
		XCode code = serviceMethod->Invoke(*message);
        this->mWaitCount--;
        return code;
	}

    void LuaRpcService::WaitAllMessageComplete()
    {
        this->mIsHandlerMessage = false;
        TaskComponent *taskComponent = this->mApp->GetTaskComponent();
        while (this->mWaitCount > 0)
        {
            taskComponent->Sleep(100);
            CONSOLE_LOG_INFO(this->GetName() <<
                " wait handler message count [" << this->mWaitCount << "]");
        }
        CONSOLE_LOG_ERROR(this->GetName() << " handler all message complete");
    }

	bool LuaRpcService::LateAwake()
	{
		LOG_CHECK_RET_FALSE(RpcService::LateAwake());
		this->mLuaComponent = this->GetComponent<LuaScriptComponent>();
		LOG_CHECK_RET_FALSE(this->mLuaEnv = this->mLuaComponent->GetLuaEnv());

        if(!Lua::Table::Get(this->mLuaEnv, this->GetName()))
        {
            LOG_ERROR(this->GetName() << " is not lua table");
            return false;
        }
        lua_pushboolean(this->mLuaEnv, false);
        lua_setfield(this->mLuaEnv, -2, "IsStartService");
		return true;
	}

	bool LuaRpcService::Close()
	{
        const char * tab = this->GetName().c_str();
        if(Lua::lua_getfunction(this->mLuaEnv, tab, "OnStop"))
        {
            if(lua_pcall(this->mLuaEnv, 0, 0, 0) != LUA_OK)
            {
                LOG_ERROR(lua_tostring(this->mLuaEnv, -1));
                return false;
            }
            return (bool)lua_toboolean(this->mLuaEnv, -1);
        }
        return true;
	}

}