//
// Created by 64658 on 2025/4/3.
//

#ifndef APP_PLAYERCOMPONENT_H
#define APP_PLAYERCOMPONENT_H
#include "Common/Entity/Player.h"
#include "Entity/Component/Component.h"
#include "Node/Component/IActorComponent.h"

namespace acs
{
	class PlayerComponent : public Component, public IActorComponent
	{
	public:
		PlayerComponent();
	public:
		Player * Get(long long playerId);
		bool Add(std::unique_ptr<Player> player);
		bool Remove(long long playerId, bool notice);
		inline size_t PlayerCount() const { return this->mPlayers.size(); }
	private:
		bool LateAwake() final;
		Actor * GetActor(long long id) final;
		int Broadcast(std::unique_ptr<rpc::Message>& message) final;
	private:
		class NodeComponent * mNode;
		std::unordered_map<long long, std::unique_ptr<Player>> mPlayers;
	};
}


#endif //APP_PLAYERCOMPONENT_H
