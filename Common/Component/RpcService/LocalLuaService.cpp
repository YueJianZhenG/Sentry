﻿#include"LocalLuaService.h"
#include"Script/Table.h"
#include"Script/Function.h"
#include"Method/LuaServiceMethod.h"
#include"Component/Lua/LuaScriptComponent.h"
#include"Global/ServiceConfig.h"
#include"Method/MethodRegister.h"
namespace Sentry
{
	LocalLuaService::LocalLuaService()
		: mLuaEnv(nullptr)
	{

	}

	LocalLuaService::~LocalLuaService()
	{
		//luaL_unref(this->mLuaEnv, LUA_REGISTRYINDEX, this->mIdx);
	}

	bool LocalLuaService::StartNewService()
	{
		this->mMethodRegister = std::make_shared<ServiceMethodRegister>(this);

		std::vector<const RpcInterfaceConfig *> rpcInterConfigs;
		const RpcServiceConfig & rpcServiceConfig = this->GetServiceConfig();
		rpcServiceConfig.GetConfigs(rpcInterConfigs);

		for(const RpcInterfaceConfig * rpcInterfaceConfig : rpcInterConfigs)
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
		return true;
	}

	XCode LocalLuaService::Invoke(const std::string& name, std::shared_ptr<com::Rpc::Request> request,
		std::shared_ptr<com::Rpc::Response> response)
	{
		if(!this->IsStartService())
		{
			LOG_ERROR(this->GetName() << " is not start");
			return XCode::CallServiceNotFound;
		}
		response->set_rpc_id(request->rpc_id());
		response->set_user_id(request->user_id());

		std::shared_ptr<ServiceMethod> serviceMethod = this->mMethodRegister->GetMethod(name);
		if (serviceMethod == nullptr)
		{
			response->set_code((int)XCode::CallServiceNotFound);
			LOG_ERROR("not find lua [" << this->GetName() << "." << name << "]");
			return XCode::CallServiceNotFound;
		}
		return serviceMethod->Invoke(*request, *response);
	}


	bool LocalLuaService::LateAwake()
	{
		LOG_CHECK_RET_FALSE(ServiceComponent::LateAwake());
		this->mLuaComponent = this->GetComponent<LuaScriptComponent>();
		LOG_CHECK_RET_FALSE(this->mLuaEnv = this->mLuaComponent->GetLuaEnv());
        if(!Lua::Table::Get(this->mLuaEnv, this->GetName()))
        {
            LOG_ERROR(this->GetName() << " is not lua table");
            return false;
        }
		return true;
	}

	bool LocalLuaService::OnStart()
	{
		const char * tab = this->GetName().c_str();
		WaitLuaTaskSource * luaTaskSource = Lua::Function::Call(this->mLuaEnv, tab, "OnStart");
		return(luaTaskSource == nullptr || luaTaskSource->Await<bool>());
	}
	bool LocalLuaService::CloseService()
	{
		return false;
	}

}