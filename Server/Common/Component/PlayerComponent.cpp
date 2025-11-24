//
// Created by 64658 on 2025/4/3.
//
#include "PlayerComponent.h"
#include "Gate/Service/GateSystem.h"
#include "Rpc/Config/ServiceConfig.h"
#include "Cluster/Config/ClusterConfig.h"
#include "Node/Component/NodeComponent.h"
namespace acs
{
	PlayerComponent::PlayerComponent()
	{
		this->mNode = nullptr;
	}

	Player* PlayerComponent::Get(long long playerId)
	{
		if(playerId <= 0)
		{
			return nullptr;
		}
		auto iter = this->mPlayers.find(playerId);
		return iter != this->mPlayers.end() ? iter->second.get() : nullptr;
	}

	bool PlayerComponent::LateAwake()
	{
		this->mNode = this->GetComponent<NodeComponent>();
		return true;
	}

	Actor* PlayerComponent::GetActor(long long playerId)
	{
		auto iter = this->mPlayers.find(playerId);
		return iter != this->mPlayers.end() ? iter->second.get() : nullptr;
	}

	int PlayerComponent::Broadcast(std::unique_ptr<rpc::Message>& message)
	{
		int count = 0;
		do
		{
			std::string func;
			if(!message->GetHead().Get(rpc::Header::func, func))
			{
				break;
			}
			const RpcMethodConfig * rpcMethodConfig = RpcConfig::Inst()->GetMethodConfig(func);
			if(rpcMethodConfig == nullptr)
			{
				break;
			}
			std::string node;
			std::string name = ComponentFactory::GetName<GateSystem>();
			if(!ClusterConfig::Inst()->GetServerName(name, node))
			{
				break;
			}
			for(int nodeId : this->mNode->GetNodes(node))
			{
				Actor * node = this->mNode->Get(nodeId);
				if(node != nullptr)
				{
					++count;
					std::unique_ptr<rpc::Message> request = message->Clone();
					{
						request->SetSockId(nodeId);
						request->SetType(rpc::type::broadcast);
						node->Send(request);
					}
				}
			}
		}
		while(false);
		return count;
	}

	bool PlayerComponent::Remove(long long playerId, bool notice)
	{
		auto iter = this->mPlayers.find(playerId);
		if(iter == this->mPlayers.end())
		{
			return false;
		}
		if(notice) {
			iter->second->Logout();
		}
		this->mPlayers.erase(iter);
		return true;
	}

	bool PlayerComponent::Add(std::unique_ptr<Player> player)
	{
		long long id = player->GetId();
		auto iter = this->mPlayers.find(id);
		if(iter != this->mPlayers.end())
		{
			return false;
		}
		if(!player->LateAwake())
		{
			return false;
		}
		this->mPlayers.emplace(id, std::move(player));
		return true;
	}
}