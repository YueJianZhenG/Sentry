#include "NodeComponent.h"
#include "Lua/Engine/ModuleClass.h"
#include "Server/Config/ServerConfig.h"
#include "Lua/Component/LuaComponent.h"
#include "Event/Base/IEvent.h"
#include "Util/Tools/Random.h"
#include "Rpc/Config/ServiceConfig.h"
namespace acs
{
	void NodeCluster::AddItem(int id)
	{
		auto iter = std::find(this->mNodes.begin(), this->mNodes.end(), id);
		if(iter == this->mNodes.end())
		{
			this->mNodes.emplace_back(id);
		}
	}

	bool NodeCluster::Remove(int id)
	{
		auto iter = std::find(this->mNodes.begin(), this->mNodes.end(), id);
		if(iter == this->mNodes.end())
		{
			return false;
		}
		this->mNodes.erase(iter);
		return true;
	}

	bool NodeCluster::Next(int& id)
	{
		if(this->mNodes.empty())
		{
			return false;
		}
		size_t index = this->mIndex++;
		if(index >= this->mNodes.size())
		{
			index = 0;
			this->mIndex = 0;
		}
		id = this->mNodes[index];
		return true;
	}

	bool NodeCluster::Random(int& id)
	{
		if(this->mNodes.empty())
		{
			return false;
		}
		size_t count = this->mNodes.size();
		size_t index = help::Rand::Random<size_t>(0, count - 1);
		id = this->mNodes.at(index);
		return true;
	}

	bool NodeCluster::Hash(long long hash, int& id)
	{
		if(this->mNodes.empty())
		{
			return false;
		}
		size_t count = this->mNodes.size();
		size_t index = hash % count;
		id = this->mNodes.at(index);
		return true;
	}


}

namespace acs
{

	NodeComponent::NodeComponent() = default;

	bool NodeComponent::LateAwake()
	{
		return this->InitNodeFromFile();
	}

	void NodeComponent::OnRecord(json::w::Document& document)
	{
		std::unique_ptr<json::w::Value> jsonObject = document.AddObject("node");
		{
			jsonObject->Add("count", this->mActors.size());
			jsonObject->Add("cluster", this->mClusters.size());
		}
	}

	bool NodeComponent::InitNodeFromFile()
	{
		auto jsonDocument = ServerConfig::Inst()->Read("machine");
		if (jsonDocument != nullptr && jsonDocument->IsArray())
		{
			json::r::Value jsonObject;
			for (size_t index = 0; index < jsonDocument->MemberCount(); index++)
			{
				if (jsonDocument->Get(index, jsonObject))
				{
					int id = 0;
					std::string name;
					json::r::Value listenObject;
					LOG_CHECK_RET_FALSE(jsonObject.Get("id", id))
					LOG_CHECK_RET_FALSE(jsonObject.Get("name", name))
					LOG_CHECK_RET_FALSE(jsonObject.Get("listen", listenObject))
					std::unique_ptr<Node> server1= std::make_unique<Node>(id, name);

					for(const char * key : listenObject.GetAllKey())
					{
						std::string address;
						LOG_CHECK_RET_FALSE(listenObject.Get(key, address))
						LOG_CHECK_RET_FALSE(server1->AddListen(key, address))
					}
					this->Add(std::move(server1));
				}
			}
		}
		return true;
	}

	Node* NodeComponent::Get(int id)
	{
		if (this->mApp->Equal(id))
		{
			return this->mApp;
		}
		auto iter = std::find_if(this->mActors.begin(), this->mActors.end(),
				[id](const std::unique_ptr<Node>& node)
				{
					return node->Equal(id);
				});
		return iter != this->mActors.end() ? (*iter).get() : nullptr;
	}

	bool NodeComponent::Remove(int id)
	{
		if(id == this->mApp->Equal(id))
		{
			return false;
		}
		auto iter = std::find_if(this->mActors.begin(), this->mActors.end(),
				[id](const std::unique_ptr<Node>& node)
				{
					return node->Equal(id);
				});
		if(iter == this->mActors.end())
		{
			return false;
		}
		const std::string name = (*iter)->Name();
		NodeCluster * nodeCluster = this->GetCluster(name);
		if(nodeCluster != nullptr)
		{
			nodeCluster->Remove(id);
		}
		this->mActors.erase(iter);
		LOG_INFO("[{}] remove node => name:{} id:{}", this->mActors.size(), name, id)
		return true;
	}

	int NodeComponent::Broadcast(std::unique_ptr<rpc::Message>& message)
	{
		int count = 0;
		do
		{
			std::string func;
			rpc::Head & head = message->GetHead();
			if(!head.Get(rpc::Header::func, func))
			{
				break;
			}
			const RpcMethodConfig * rpcMethodConfig = RpcConfig::Inst()->GetMethodConfig(func);
			if(rpcMethodConfig == nullptr)
			{
				return XCode::NotFoundRpcConfig;
			}
			for(std::unique_ptr<Node> & actor : this->mActors)
			{
				std::unique_ptr<rpc::Message> rpcMessage = message->Clone();
				{
					rpcMessage->SetSockId(actor->GetNodeId());
					if(actor->Send(rpcMessage) == XCode::Ok)
					{
						++count;
					}
				}
			}
		}
		while(false);
		return count;
	}

	std::vector<int> NodeComponent::GetNodes()
	{
		std::vector<int> nodes;
		nodes.reserve(this->mActors.size());
		for(std::unique_ptr<Node> & node : this->mActors)
		{
			nodes.emplace_back(node->GetNodeId());
		}
		return nodes;
	}

	std::vector<int> NodeComponent::GetNodes(const std::string& node)
	{
		std::vector<int> nodes;
		NodeCluster* nodeCluster = this->GetCluster(node);
		if (nodeCluster == nullptr)
		{
			return nodes;
		}
		const std::vector<int>& list = nodeCluster->GetNodes();
		nodes.insert(nodes.end(), list.begin(), list.end());
		return nodes;
	}

	bool NodeComponent::AddCluster(const std::string& name, int id)
	{
		if(name.empty() || id == 0)
		{
			return false;
		}
		NodeCluster * oldCluster = this->GetCluster(name);
		if(oldCluster == nullptr)
		{
			std::unique_ptr<NodeCluster> newCluster = std::make_unique<NodeCluster>(name);
			{
				newCluster->AddItem(id);
				this->mClusters.emplace_back(std::move(newCluster));
			}
			return true;
		}
		oldCluster->AddItem(id);
		return true;
	}

	bool NodeComponent::GetListen(int id, const std::string& net, std::string& address)
	{
		Node * actor = this->Get(id);
		if(actor == nullptr)
		{
			return false;
		}
		return actor->GetListen(net, address);
	}

	NodeCluster* NodeComponent::GetCluster(const std::string& name)
	{
		auto iter = std::find_if(this->mClusters.begin(), this->mClusters.end(),
				[&name](const std::unique_ptr<NodeCluster>& cluster)
				{
					return cluster->Name() == name;
				});
		return iter == this->mClusters.end() ? nullptr : (*iter).get();
	}

	Node* NodeComponent::Next(const std::string& name)
	{
		NodeCluster * actorGroup = this->GetCluster(name);
		if(actorGroup == nullptr)
		{
			return nullptr;
		}
		int id = 0;
		if(!actorGroup->Next(id))
		{
			return nullptr;
		}
		return this->Get(id);
	}

	Node* NodeComponent::Hash(const std::string& name, long long key)
	{
		NodeCluster* actorGroup = this->GetCluster(name);
		if(actorGroup == nullptr)
		{
			return nullptr;
		}
		int id = 0;
		if(!actorGroup->Hash(key, id))
		{
			return nullptr;
		}
		return this->Get(id);
	}

	Node* NodeComponent::Rand(const std::string& name)
	{
		NodeCluster* actorGroup = this->GetCluster(name);
		if(actorGroup == nullptr)
		{
			return nullptr;
		}
		int id = 0;
		if(!actorGroup->Random(id))
		{
			return nullptr;
		}
		return this->Get(id);
	}

	Actor* NodeComponent::GetActor(long long id)
	{
		if(this->mApp->Equal(id))
		{
			return this->mApp;
		}
		auto iter = std::find_if(this->mActors.begin(), this->mActors.end(),
				[id](const std::unique_ptr<Node>& node)
				{
					return node->Equal(id);
				});
		return iter != this->mActors.end() ? (*iter).get() : nullptr;
	}

	bool NodeComponent::Add(std::unique_ptr<Node> node)
	{
		int id = node->GetNodeId();
		auto iter = std::find_if(this->mActors.begin(), this->mActors.end(),
				[id](const std::unique_ptr<Node>& node)
				{
					return node->Equal(id);
				});
		if(iter != this->mActors.end() || node->Name().empty())
		{
			return false;
		}
		if(!node->LateAwake())
		{
			return false;
		}
		std::string name(node->Name());
		this->AddCluster(node->Name(), id);
		LOG_DEBUG("add node => {}", node->ToString())
		this->mActors.emplace_back(std::move(node));
		return true;
	}
}