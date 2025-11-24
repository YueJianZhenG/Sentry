//
// Created by leyi on 2023/5/23.
//

#include "Node.h"
#include "Entity/Actor/App.h"
#include "Http/Common/HttpResponse.h"
#include "Util/Tools/String.h"
#include "Rpc/Config/ServiceConfig.h"
#include "Router/Component/RouterComponent.h"
namespace acs
{
	Node::Node(int id, const std::string & name)
		: Actor(id, name), mRpc("rpc")
	{
		this->mAppId = 0;
	}

	bool Node::OnInit()
	{
		this->mAppId = App::Inst()->GetId();
		return true;
	}

	bool Node::GetAddress(const rpc::Message & request, int & id) const
	{
		id = this->GetNodeId();
		return true;
	}

	int Node::DisConnect()
	{
		std::unique_ptr<rpc::Message> message = std::make_unique<rpc::Message>();
		{
			message->SetNet(rpc::net::tcp);
			message->SetType(rpc::type::logout);
		}
		return this->mRouter->Send(this->GetNodeId(), message);
	}

	bool Node::AddListen(const std::string& name, const std::string& addr)
	{
		LOG_CHECK_RET_FALSE(!addr.empty() && !name.empty());

		std::string ip, net;
		unsigned short port = 0;
		if (!help::Str::SplitAddr(addr, ip, port))
		{
			if (!help::Str::SplitAddr(addr, net, ip, port))
			{
				LOG_ERROR("add {} {} {} fail", this->Name(), name, addr);
				return false;
			}
		}
		auto iter = std::find_if(this->mListens.begin(), this->mListens.end(),
				[&name](const std::pair<std::string, std::string>& item)
				{
					return item.first == name;
				});
		std::string data = fmt::format("{0}:{1}", ip, port);
		if (iter != this->mListens.end() && iter->second == data)
		{
			return false;
		}
		this->mListens.emplace_back(name, addr);
		return true;
	}

	bool Node::GetListen(const std::string& name, std::string& addr) const
	{
		auto iter = std::find_if(this->mListens.begin(), this->mListens.end(),
				[&name](const std::pair<std::string, std::string>& item)
				{
					return item.first == name;
				});
		if (iter == this->mListens.end())
		{
			LOG_WARN("{} not {} address", this->Name(), name);
			return false;
		}
		addr = iter->second;
		return true;
	}

	int Node::SendMsg(std::unique_ptr<rpc::Message> message)
	{
		int id = this->GetNodeId();
		return this->mRouter->Send(id, message);
	}

	std::string Node::ToString()
	{
		json::w::Document document;
		document.Add("id", this->GetNodeId());
		std::unique_ptr<json::w::Value> listenArray = document.AddObject("listen");
		for (auto iter = this->mListens.begin(); iter != this->mListens.end(); iter++)
		{
			listenArray->Add(iter->first.c_str(), iter->second);
		}
		return document.JsonString();
	}

	std::unique_ptr<rpc::Message> Node::Make(const std::string& func) const
	{
		const RpcMethodConfig * methodConfig = RpcConfig::Inst()->GetMethodConfig(func);
		if(methodConfig == nullptr)
		{
			LOG_ERROR("not rpc config {}", func);
			return nullptr;
		}
		std::unique_ptr<rpc::Message> message = std::make_unique<rpc::Message>();
		{
			message->SetNet(methodConfig->net);
			message->SetType(rpc::type::request);
			message->SetProto(methodConfig->proto);
			message->GetHead().Add(rpc::Header::func, func);
			message->GetHead().Add(rpc::Header::id, this->mAppId);
			message->SetTimeout(methodConfig->timeout);
		}
		message->SetSockId(this->GetNodeId());
		return message;
	}
}