//
// Created by yjz on 2023/5/17.
//

#ifndef APP_NODECOMPONENT_H
#define APP_NODECOMPONENT_H

#include "Node/Actor/Node.h"
#include "Core/Map/HashMap.h"
#include "IActorComponent.h"
namespace Lua
{
	class LuaModule;
};

namespace acs
{
	class NodeCluster
	{
	public:
		explicit NodeCluster(std::string name) : mName(std::move(name)), mIndex() { }
	public:
		bool Remove(int id);
		bool Next(int & id);
		void AddItem(int id);
		bool Random(int & id);
		bool Hash(long long hash, int & id);
		inline const std::string & Name() const { return this->mName; }
		const std::vector<int> & GetNodes() const { return this->mNodes; }
	private:
		size_t mIndex;
		std::string mName;
		std::vector<int> mNodes;
	};
}

namespace acs
{
	class NodeComponent final : public Component, public IServerRecord, public IActorComponent
	{
	public:
		NodeComponent();
	public:
		Node * Get(int id);
		Node * Next(const std::string & name);
		Node * Rand(const std::string & name);
		Node * Hash(const std::string & name, long long key);
	public:
		bool Remove(int id);
		bool Add(std::unique_ptr<Node> node);
	public:
		std::vector<int> GetNodes();
		std::vector<int> GetNodes(const std::string & node);
	public:
		bool AddCluster(const std::string & name, int id);
		NodeCluster * GetCluster(const std::string & name);
		inline size_t GetActorCount() const { return this->mActors.size(); }
		bool GetListen(int id, const std::string & net, std::string & address);
		int Broadcast(std::unique_ptr<rpc::Message> & message) final;
	private:
		bool LateAwake() final;
		bool InitNodeFromFile();
		Actor * GetActor(long long id) final;
		void OnRecord(json::w::Document &document) final;
	private:
		std::vector<std::unique_ptr<Node>> mActors;
		std::vector<std::unique_ptr<NodeCluster>> mClusters;
	};
}

#endif //APP_NODECOMPONENT_H
