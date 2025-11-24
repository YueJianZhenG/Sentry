//
// Created by zmhy0073 on 2021/12/1.
//

#include "GateSystem.h"
#include "Entity/Actor/App.h"
#include "Core/System/System.h"
#include "Event/Base/IEvent.h"
#include "Auth/Jwt/Jwt.h"
#include "Common/Service/LoginSystem.h"
#include "Cluster/Config/ClusterConfig.h"
#include "Util/Tools/TimeHelper.h"
#include "Common/Component/PlayerComponent.h"
namespace acs
{
    GateSystem::GateSystem()
    {
		this->mPlayerMgr = nullptr;
    }

    bool GateSystem::OnInit()
    {
		BIND_RPC_METHOD(GateSystem::Ping);
		BIND_RPC_METHOD(GateSystem::Login);
		BIND_RPC_METHOD(GateSystem::Logout);
		BIND_RPC_METHOD(GateSystem::Create);
		this->mPlayerMgr = this->GetComponent<PlayerComponent>();
		return true;
    }

	int GateSystem::Ping(long long userId)
	{
		if(this->mPlayerMgr->Get(userId) == nullptr)
		{
			return XCode::NotFindUser;
		}
		return XCode::Ok;
	}

	bool GateSystem::AllotServer(std::vector<int>& servers)
	{
		return true;
	}

	int GateSystem::Login(const rpc::Message & request)
	{
		int sockId = 0;
		const rpc::Head & head = request.ConstHead();
		if(!head.Get(rpc::Header::client_sock_id, sockId))
		{
			return XCode::CallArgsError;
		}
		const std::string & token = request.GetBody();

		json::r::Document document;
		if(!jwt::Decode(token, document))
		{
			return XCode::Failure;
		}
		long long userId, expTime = 0;
		long long nowTime = help::Time::NowSec();
		LOG_ERROR_CHECK_ARGS(document.Get("user_id", userId))
		LOG_ERROR_CHECK_ARGS(document.Get("end_time", expTime))
		if(nowTime >= expTime)
		{
			return XCode::TokenExpTime;
		}
		if(this->mPlayerMgr->Get(userId) != nullptr)
		{
			return XCode::NotFindUser;
		}
		s2s::login::create message;
		{
			message.set_user_id(userId);
			message.set_sock_id(sockId);
		}
		return this->Create(message);
	}

	int GateSystem::Create(const s2s::login::create& request)
	{
		std::vector<int> servers;
		if(!this->AllotServer(servers))
		{
			return XCode::AddressAllotFailure;
		}
		int sockId = request.sock_id();
		long long userId = request.user_id();
		int serverId = this->mApp->GetNodeId();
		std::unique_ptr<Player> player = std::make_unique<Player>(userId, serverId, sockId);
		{
			s2s::login::request message;
			message.set_user_id(userId);
			message.set_client_id(sockId);
			player->AddNode(this->mApp->Name(), serverId);
			// 分配游戏服

			s2s::server::info * serverInfo = message.add_list();
			{
				serverInfo->set_id(serverId);
				serverInfo->set_name(this->mApp->Name());
			}

			for(const int allotId : servers)
			{
				Actor * server = this->mPlayerMgr->Get(allotId);
				if(server != nullptr && allotId != serverId)
				{
					if(server->Call("LoginSystem.Login", message) != XCode::Ok)
					{
						return XCode::Failure;
					}
					player->AddNode(server->Name(), (int)server->GetId());
				}
			}
		}
		this->mPlayerMgr->Add(std::move(player));
		help::PlayerLoginGateEvent::Trigger(userId, sockId);
		//LOG_INFO("user:({}) login to gate successful", userId);
		return XCode::Ok;
	}

	int GateSystem::Logout(const rpc::Message & request)
	{
		int sockId = 0;
		long long userId = 0;
		const rpc::Head & head = request.ConstHead();
		if(!head.Get(rpc::Header::client_sock_id, sockId))
		{
			LOG_ERROR("not find client_sock_id");
			return XCode::CallArgsError;
		}
		head.Get(rpc::Header::id, userId);
		Actor * player = this->mPlayerMgr->Get(userId);
		if(player == nullptr)
		{
			LOG_ERROR("not find player_id:{}", userId);
			return XCode::NotFindUser;
		}
		help::PlayerLogoutGateEvent::Trigger(userId, sockId);
		return XCode::CloseSocket;
	}
}