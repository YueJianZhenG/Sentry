﻿#include"LuaPhysicalRpcService.h"
#include"Lua/Function.h"
#include"Module/LuaModule.h"
#include"Lua/LuaServiceMethod.h"
#include"Config/ServiceConfig.h"
#include"Method/MethodRegister.h"
#include"Component/LuaScriptComponent.h"
namespace Sentry
{
	LuaPhysicalRpcService::LuaPhysicalRpcService()
		: mMethodRegister(this)
	{
		this->mSumCount = 0;
		this->mWaitCount = 0;
		this->mUserCount = 0;
        this->mLuaComponent = nullptr;
        this->mIsHandlerMessage = false;
	}

	bool LuaPhysicalRpcService::Init()
	{
		this->mLuaComponent = this->GetComponent<LuaScriptComponent>();
		if(this->mLuaComponent == nullptr)
		{
			return false;
		}
		const std::string & name = this->GetName();
		if(this->mLuaComponent->LoadModule(name) == nullptr)
		{
			return false;
		}

        this->mIsHandlerMessage = true;
        return this->LoadFromLua();
	}

	bool LuaPhysicalRpcService::LoadFromLua()
	{
		const std::string & name = this->GetName();
		Lua::LuaModule * luaModule = this->mLuaComponent->LoadModule(name);
		if(luaModule == nullptr)
		{
			LOG_ERROR("not find lua rpc module [" << name << "]");
			return false;
		}
		std::vector<const RpcMethodConfig *> rpcInterConfigs;
		const RpcServiceConfig * rpcServiceConfig = RpcConfig::Inst()->GetConfig(name);
		LOG_CHECK_RET_FALSE(rpcServiceConfig && rpcServiceConfig->GetMethodConfigs(rpcInterConfigs) > 0);

		for(const RpcMethodConfig * rpcInterfaceConfig : rpcInterConfigs)
		{
			if(!luaModule->GetFunction(rpcInterfaceConfig->Method))
			{
				LOG_ERROR("not find lua rpc method [" << rpcInterfaceConfig->FullName << "]");
				return false;
			}
			this->mMethodRegister.AddMethod(
				std::make_shared<LuaServiceMethod>(rpcInterfaceConfig));
		}
		return true;
	}

	int LuaPhysicalRpcService::Invoke(const std::string &name, std::shared_ptr<Rpc::Packet> message)
	{
		if(!this->mIsHandlerMessage || !this->IsStartService())
		{
			CONSOLE_LOG_ERROR(this->GetName() << " is not start");
			return XCode::CallServiceNotFound;
		}
		std::shared_ptr<ServiceMethod> serviceMethod = this->mMethodRegister.GetMethod(name);
		if (serviceMethod == nullptr)
		{
			return XCode::CallServiceNotFound;
		}
        this->mWaitCount++;
		int code = serviceMethod->Invoke(*message);
        this->mWaitCount--;
        return code;
	}

    void LuaPhysicalRpcService::WaitAllMessageComplete()
    {
        this->mIsHandlerMessage = false;
        AsyncMgrComponent *taskComponent = this->mApp->GetTaskComponent();
        while (this->mWaitCount > 0)
        {
            taskComponent->Sleep(100);
            CONSOLE_LOG_INFO(this->GetName() <<
                " wait handler message count [" << this->mWaitCount << "]");
        }
        CONSOLE_LOG_ERROR(this->GetName() << " handler all message complete");
    }

	void LuaPhysicalRpcService::OnLogin(long long userId)
	{
		const std::string & name = this->GetName();
		Lua::LuaModule * luaModule = this->mLuaComponent->GetModule(name);
		if(luaModule != nullptr)
		{
			this->mUserCount++;
			luaModule->Invoke("OnLogin", userId);
		}
	}

	void LuaPhysicalRpcService::OnLogout(long long userId)
	{
		const std::string & name = this->GetName();
		Lua::LuaModule * luaModule = this->mLuaComponent->GetModule(name);
		if(luaModule != nullptr)
		{
			this->mUserCount--;
			luaModule->Invoke("OnLogout", userId);
		}
	}

	bool LuaPhysicalRpcService::Close()
	{
		if(this->mLuaComponent != nullptr)
		{
			const std::string & name = this->GetName();
			return this->mLuaComponent->UnloadModule(name);
		}
		return true;
	}

	void LuaPhysicalRpcService::OnRecord(Json::Writer& document)
	{
		document.Add("sum").Add(this->mSumCount);
		document.Add("wait").Add(this->mWaitCount);
		document.Add("client").Add(this->mUserCount);
	}
	bool LuaPhysicalRpcService::Start()
	{
		const std::string & name = this->GetName();
		Lua::LuaModule * luaModule = this->mLuaComponent->GetModule(name);
		return luaModule != nullptr && luaModule->Start();
	}
}