//
// Created by leyi on 2023/5/15.
//

#include"Player.h"
#include "Rpc/Config/ServiceConfig.h"
#include"Entity/Actor/App.h"
#include"Server/Config/ServerConfig.h"
#include "Util/Tools/TimeHelper.h"
#include "s2s/s2s.pb.h"

namespace acs
{
	Player::Player(long long playerId, int gate, int sockId)
		: Actor(playerId, "PLayer"), mGateId(gate), mSockId(sockId)
	{
		this->mActor = App::ActorMgr();
	}

	bool Player::OnInit()
	{
		if(this->mActor->Get(this->mGateId) == nullptr)
		{
			LOG_ERROR("not find gate actor : {}", this->mGateId);
			return false;
		}
		return true;
	}

	void Player::Logout()
	{
		for (auto iter = this->mServerAddrs.begin(); iter != this->mServerAddrs.end(); iter++)
		{
			s2s::logout::request request;
			request.set_user_id(this->GetId());
			Actor* server = this->mActor->Get(iter->second);
			if (server != nullptr)
			{
				server->Send("LoginSystem.Logout", request);
			}
		}
	}

	bool Player::DelNode(const std::string& server)
	{
		auto iter = std::find_if(this->mServerAddrs.begin(), this->mServerAddrs.end(),
				[server](const std::pair<std::string, int>& item)
				{
					return item.first == server;
				});
		if (iter == this->mServerAddrs.end())
		{
			return false;
		}
		this->mServerAddrs.erase(iter);
		return true;
	}

	void Player::AddNode(const std::string& server, int id)
	{
		if(server.empty())
		{
			return;
		}
		auto iter = std::find_if(this->mServerAddrs.begin(), this->mServerAddrs.end(),
				[server](const std::pair<std::string, int>& item)
				{
					return item.first == server;
				});
		if(iter == this->mServerAddrs.end())
		{
			iter->second = id;
			return;
		}
		this->mServerAddrs.emplace_back(server, id);
	}

	bool Player::GetNodeID(const std::string& server, int & id) const
	{
		auto iter = std::find_if(this->mServerAddrs.begin(), this->mServerAddrs.end(),
				[server](const std::pair<std::string, int>& item)
				{
					return item.first == server;
				});
		if (iter == this->mServerAddrs.end())
		{
			return false;
		}
		id = iter->second;
		return true;
	}

	bool Player::GetAddress(const rpc::Message& request, int & id) const
	{
		const std::string & func = request.ConstHead().GetStr(rpc::Header::func);
		const RpcMethodConfig * methodConfig = RpcConfig::Inst()->GetMethodConfig(func);
		return methodConfig != nullptr && this->GetNodeID(methodConfig->node, id);
	}


	std::unique_ptr<rpc::Message> Player::Make(const std::string& func) const
	{
		const RpcMethodConfig * methodConfig = RpcConfig::Inst()->GetMethodConfig(func);
		if(methodConfig == nullptr)
		{
			LOG_ERROR("not rpc config {}", func);
			return nullptr;
		}
		int serverId = 0;
		std::unique_ptr<rpc::Message> message = std::make_unique<rpc::Message>();
		{
			if(methodConfig->to_client)
			{
				serverId = this->mGateId;
				message->SetNet(methodConfig->net);
				message->SetType(rpc::type::client);
				message->GetHead().Add(rpc::Header::client_sock_id, this->mSockId);
			}
			else if(this->GetNodeID(methodConfig->node, serverId))
			{
				message->SetNet(methodConfig->net);
				message->SetType(rpc::type::request);
			}
			message->SetSockId(serverId);
			message->SetProto(methodConfig->proto);
			message->GetHead().Add(rpc::Header::func , func);
			message->GetHead().Add(rpc::Header::id, this->GetId());
		}
		return message;
	}
}