﻿#pragma once


#include<Script/LuaInclude.h>
#include"ServiceBase/ServiceComponent.h"

class LuaTable;
namespace GameKeeper
{
    class LuaScriptComponent;

    class LuaServiceComponent : public ServiceComponent, public IStart
    {
    public:
        LuaServiceComponent();
        ~LuaServiceComponent() override;
	public:
		bool InitService(const std::string & name, lua_State * luaEnv);
    public:
        bool Awake() final;
        void OnStart() final;
        bool LateAwake() final;
		const std::string &GetServiceName()final { return this->mServiceName; }
    private:
		int mIdx;
		lua_State * mLuaEnv;
		std::string mServiceName;
        std::shared_ptr<LuaTable> mLuaTable;
        class RpcConfigComponent * mRpcConfigComponent;
        std::unordered_map<std::string, int> mMethodMap;
    };
}// namespace GameKeeper