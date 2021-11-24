﻿#pragma once

#include "Service/ServiceComponent.h"
#include <Script/LuaInclude.h>

namespace GameKeeper
{
    class LuaScriptComponent;

    class LuaServiceComponent : public ServiceComponent
    {
    public:
        LuaServiceComponent();

        ~LuaServiceComponent() override;
	public:
		bool InitService(const std::string & name, lua_State * luaEnv);

    public:
        bool Awake() final;
		void Start() final;
		bool IsLuaService() final { return true; };
		const std::string &GetServiceName()final { return this->mServiceName; }
        
    private:
		int mIdx;
		lua_State * mLuaEnv;
		std::string mServiceName;
    };
}// namespace GameKeeper