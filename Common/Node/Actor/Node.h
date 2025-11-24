//
// Created by leyi on 2023/5/23.
//

#ifndef APP_NODE_H
#define APP_NODE_H
#include "Entity/Actor/Actor.h"

namespace acs
{
	class Node : public Actor
	{
	public:
		explicit Node(int id, const std::string & name);
		inline bool Equal(long long id) const { return this->GetId() == id; }
	public:
		int DisConnect();
		std::string ToString();
		int SendMsg(std::unique_ptr<rpc::Message> message);
		bool GetAddress(const rpc::Message &request, int &) const final;
		bool GetListen(const std::string & name, std::string & addr) const;
		bool AddListen(const std::string & name, const std::string & addr);
	public:
		bool OnInit() final;
		int GetNodeId() const { return (int)this->GetId(); }
		std::unique_ptr<rpc::Message> Make(const std::string &func) const final ;
		const std::vector<std::pair<std::string, std::string>> & GetListens() const { return this->mListens; }
	private:
		long long mAppId;
		std::string mRpc;
		std::vector<std::pair<std::string, std::string>> mListens;
	};
}


#endif //APP_NODE_H
