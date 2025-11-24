//
// Created by zmhy0073 on 2022/10/13.
//

#include "LoginSystem.h"
#include "Event/Base/EventID.h"
#include "Lua/Module/LuaModule.h"
#include "Common/Component/PlayerComponent.h"
#include "Event/Component/EventProxyComponent.h"
namespace acs
{
	LoginSystem::LoginSystem()
	{
		this->mPlayerMgr = nullptr;
		this->mEventProxy = nullptr;
	}

	bool LoginSystem::OnInit()
	{
		BIND_RPC_METHOD(LoginSystem::Login);
		BIND_RPC_METHOD(LoginSystem::Logout);
		this->mPlayerMgr = this->GetComponent<PlayerComponent>();
		this->mEventProxy = this->GetComponent<EventProxyComponent>();
		return true;
	}

    int LoginSystem::Login(long long id, const s2s::login::request & request)
	{
		int gateId = (int)id;
		int sockId = request.client_id();
		long long playerId = request.user_id();
		if(this->mPlayerMgr->Get(playerId) == nullptr)
		{
			std::unique_ptr<Player> player = std::make_unique<Player>(playerId, gateId, sockId);
			{
				for (int index = 0; index < request.list_size(); index++)
				{
					const s2s::server_info & serverInfo = request.list(index);
					player->AddNode(serverInfo.name(), serverInfo.id());
				}
			}
		}
		this->mEventProxy->Trigger(event::OnPlayerLogin, playerId);
		return XCode::Ok;
	}

    int LoginSystem::Logout(long long id, const s2s::logout::request& request)
    {
		long long playerId = request.user_id();
		Player * player = this->mPlayerMgr->Get(playerId);
		if(player == nullptr)
		{
			return XCode::NotFindUser;
		}
		this->mPlayerMgr->Remove(playerId, true);
		this->mEventProxy->Trigger(event::OnPlayerLogout, playerId);
		return XCode::Ok;
    }
}