//
// Created by yy on 2025/10/23.
//

#include "LuaEventComponent.h"
#include "EventProxyComponent.h"
#include "Lua/Component/LuaComponent.h"
namespace acs
{
	LuaEventComponent::LuaEventComponent()
	{
		this->mLuaProxy = nullptr;
		this->mEventProxy = nullptr;
	}

	bool LuaEventComponent::LateAwake()
	{
		this->mLuaProxy = this->mApp->GetComponent<LuaComponent>();
		this->mEventProxy = this->GetComponent<EventProxyComponent>();
		LOG_CHECK_RET_FALSE(this->mLuaProxy != nullptr && this->mEventProxy);
		this->mEventProxy->SubEvent("OnPlayerLogin", this, &LuaEventComponent::OnPlayerLogin);
		return true;
	}

	void LuaEventComponent::OnPlayerLogin(long long playerId)
	{

	}


}