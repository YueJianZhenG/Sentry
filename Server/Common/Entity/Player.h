//
// Created by leyi on 2023/5/15.
//

#ifndef APP_PLAYER_H
#define APP_PLAYER_H
#include"Entity/Actor/Actor.h"

namespace acs
{
	class Player final : public Actor
	{
	public:
		Player(long long playerId, int gate, int sockId);
	public:
		inline int GetGateId() const { return this->mGateId; }
		inline int GetSocketId() const { return this->mSockId; }
	public:
		void Logout();
		bool OnInit() final;
		bool GetNodeID(const std::string & srv, int & id) const;
		bool GetAddress(const rpc::Message &request, int & id) const final;
	public:
		bool DelNode(const std::string & server);
		void AddNode(const std::string & server, int id);
	protected:
		std::unique_ptr<rpc::Message> Make(const std::string &func) const final;
	private:
		int mGateId; //所在网关id
		int mSockId; //所在网关分配的id
		class NodeComponent * mActor;
		std::vector<std::pair<std::string, int>> mServerAddrs; //玩家所在的所有节点名字和对应id
	};
}


#endif //APP_PLAYER_H
