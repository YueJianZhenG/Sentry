//
// Created by mac on 2022/4/6.
//

#include "Method/MethodRegister.h"

#include"LocalService.h"
#include"App/App.h"
#include"Config/ServiceConfig.h"
#include"Lua/LuaServiceMethod.h"
#include"Component/InnerNetComponent.h"
#include"Component/LuaScriptComponent.h"

namespace Sentry
{
    extern std::string GET_FUNC_NAME(std::string fullName)
    {
        size_t pos = fullName.find("::");
        return fullName.substr(pos + 2);
    }
    XCode LocalService::Invoke(const std::string &func, std::shared_ptr<Rpc::Data> message)
	{
		if (!this->IsStartService())
		{
			LOG_ERROR(this->GetName() << " is not start");
			return XCode::CallServiceNotFound;
		}

		std::shared_ptr<ServiceMethod> serviceMethod = this->mMethodRegister->GetMethod(func);
		if (serviceMethod == nullptr)
		{
			LOG_ERROR("not find [" << this->GetName() << "." << func << "]");
			return XCode::CallServiceNotFound;
		}
#ifndef __DEBUG__
        message->GetHead().Remove("func");
#endif
		try
		{
			XCode code = serviceMethod->Invoke(*message);

            message->SetType(Tcp::Type::Response);
            message->GetHead().Remove("address");
            message->GetHead().Add("code", (int)code);
		}
		catch (std::logic_error & error)
		{
            message->SetType(Tcp::Type::Response);
            message->GetHead().Remove("address");
            message->GetHead().Add("error", error.what());
            message->GetHead().Add("code", (int)XCode::ThrowError);
            return XCode::ThrowError;
		}
	}

	bool LocalService::StartNewService()
	{
		this->mMethodRegister = std::make_shared<ServiceMethodRegister>(this);
		if (!this->OnStartService())
		{
			return false;
		}
        std::vector<const RpcInterfaceConfig *> rpcInterfaceConfigs;
        this->GetServiceConfig().GetConfigs(rpcInterfaceConfigs);
		LuaScriptComponent* luaScriptComponent = this->GetComponent<LuaScriptComponent>();

		if (luaScriptComponent != nullptr)
		{
            lua_State * lua = luaScriptComponent->GetLuaEnv();
            for(const RpcInterfaceConfig * rpcInterfaceConfig : rpcInterfaceConfigs)
            {
                const char* tab = rpcInterfaceConfig->Service.c_str();
                const char * func = rpcInterfaceConfig->Method.c_str();
                if (Lua::Function::Get(lua, tab, func))
                {
                    std::shared_ptr<LuaServiceMethod> luaServiceMethod
                            = std::make_shared<LuaServiceMethod>(rpcInterfaceConfig, lua);
                    this->mMethodRegister->AddMethod(luaServiceMethod);
                }
                else if (this->mMethodRegister->GetMethod(rpcInterfaceConfig->Method) == nullptr)
                {
                    return false;
                }
            }
		}

        for(const RpcInterfaceConfig * config : rpcInterfaceConfigs)
        {
            if(this->mMethodRegister->GetMethod(config->Method) == nullptr)
            {
                LOG_ERROR("not register " << config->FullName);
                return false;
            }
        }
        this->AddHost(this->GetApp()->GetConfig().GetLocalHost());
		return true;
	}

	bool LocalService::CloseService()
	{
		if(this->IsStartService())
		{
			std::move(this->mMethodRegister);
			return true;
		}
		return false;
	}
}