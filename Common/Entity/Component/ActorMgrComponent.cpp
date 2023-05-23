//
// Created by yjz on 2023/5/17.
//

#include"ActorMgrComponent.h"
#include"Entity/Actor/App.h"
#include"Server/Config/ServerConfig.h"
namespace Tendo
{
	bool ActorMgrComponent::AddRandomActor(const std::string& name, Tendo::Actor* actor)
	{
		if (name.empty())
		{
			return false;
		}
		long long actorId = actor->GetUnitId();
		auto iter1 = this->mActorNames.find(name);
		if (iter1 == this->mActorNames.end())
		{
			std::vector<long long> ret;
			this->mActorNames.emplace(name, ret);
		}
		this->mActorNames[name].emplace_back(actorId);
		return true;
	}

	Actor* ActorMgrComponent::RandomActor(const std::string& name)
	{
		auto iter = this->mActorNames.find(name);
		if(iter == this->mActorNames.end())
		{
			return nullptr;
		}
		int index = rand() % (int)iter->second.size();
		return this->GetActor(iter->second[index]);
	}

	Actor* ActorMgrComponent::GetActor(long long id)
	{
		auto iter = this->mServers.find(id);
		if(iter != this->mServers.end())
		{
			return iter->second.get();
		}
		auto iter1 = this->mPlayers.find(id);
		return iter1 != this->mPlayers.end() ? iter1->second.get() : nullptr;
	}

	bool ActorMgrComponent::DelActor(long long id)
	{
		auto iter = this->mPlayers.find(id);
		if(iter != this->mPlayers.end())
		{
			this->mPlayers.erase(iter);
			return true;
		}
		auto iter1 = this->mServers.find(id);
		if(iter1 != this->mServers.end())
		{
			this->mServers.erase(iter1);
			return true;
		}
		return false;
	}

	bool ActorMgrComponent::AddPlayer(std::shared_ptr<Player> player)
	{
		long long playerId = player->GetActorId();
		auto iter = this->mPlayers.find(playerId);
		if(iter != this->mPlayers.end() || !player->LateAwake())
		{
			return false;
		}
		assert(player->GetActorId() > 10000);
		this->mPlayers.emplace(playerId, player);
		return true;
	}

	Player* ActorMgrComponent::GetPlayer(long long playerId)
	{
		auto iter = this->mPlayers.find(playerId);
		return iter != this->mPlayers.end() ? iter->second.get() : nullptr;
	}

	Server* ActorMgrComponent::GetServer(long long serverId)
	{
		auto iter = this->mServers.find(serverId);
		return iter != this->mServers.end() ? iter->second.get() : nullptr;
	}

	bool ActorMgrComponent::AddServer(std::shared_ptr<Server> server)
	{
		assert(server->GetActorId() < 10000);
		long long serverId = server->GetActorId();
		const std::string & name = server->Name();
		auto iter = this->mServers.find(serverId);
		if(name.empty() || iter != this->mServers.end() || !server->LateAwake())
		{
			return false;
		}
		this->AddRandomActor(name, server.get());
		this->mServers.emplace(serverId, std::move(server));
		return true;
	}

	void ActorMgrComponent::GetServers(std::vector<Server *>& servers)
	{
		auto iter = this->mServers.begin();
		for(; iter != this->mServers.end(); iter++)
		{
			servers.emplace_back(iter->second.get());
		}
	}
}