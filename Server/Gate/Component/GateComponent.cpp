//
// Created by leyi on 2023/9/11.
//
#include "XCode/XCode.h"
#include "GateComponent.h"
#include "Entity/Actor/App.h"
#include "Common/Entity/Player.h"
#include "Util/Tools/TimeHelper.h"
#include "Rpc/Config/MethodConfig.h"
#include "Rpc/Config/ServiceConfig.h"
#include "Node/Component/NodeComponent.h"
#include "Common/Component/PlayerComponent.h"
#include "Router/Component/RouterComponent.h"


namespace acs
{
	GateComponent::GateComponent()
	{
		this->mNode = nullptr;
		this->mRouter = nullptr;
		this->mOuterComponent = nullptr;
		this->mPlayerComponent = nullptr;
	}

	bool GateComponent::LateAwake()
	{
		std::vector<rpc::IOuterSender *> outerSenders;
		LOG_CHECK_RET_FALSE(this->mApp->GetComponents(outerSenders) == 1);
		LOG_CHECK_RET_FALSE(this->mNode = this->GetComponent<NodeComponent>())
		LOG_CHECK_RET_FALSE(this->mRouter = this->GetComponent<RouterComponent>())
		LOG_CHECK_RET_FALSE(this->mPlayerComponent = this->GetComponent<PlayerComponent>())
		LOG_CHECK_RET_FALSE(this->mOuterComponent = this->mApp->GetComponent<rpc::IOuterSender>())
		return true;
	}

	int GateComponent::Send(int id, std::unique_ptr<rpc::Message> & message)
	{
		return this->mOuterComponent->Send(id, message);
	}

	void GateComponent::Broadcast(std::unique_ptr<rpc::Message> & message)
	{

	}

	int GateComponent::OnMessage(std::unique_ptr<rpc::Message> & message) noexcept
	{
#ifdef __DEBUG__
		rpc::Head& head = message->GetHead();
		long long nowTime = help::Time::NowMil();
#endif
		switch (message->GetType())
		{
			case rpc::type::request:
#ifdef __DEBUG__
				head.Add("t1", nowTime);
#endif
				this->OnRequest(message);
				break;
			case rpc::type::response:
			{
#ifdef __DEBUG__
				std::string func;
				long long startTime = 0;
				head.Get("t1", startTime);
				head.Get(rpc::Header::func, func);
				LOG_INFO("call [{}] use time => {}ms", func, nowTime - startTime);
#endif
				this->OnResponse(message);
			}
				break;
			default:
				LOG_ERROR("unknown message {}", message->ToString());
				break;
		}
		return XCode::Ok;
	}

	int GateComponent::OnRequest(std::unique_ptr<rpc::Message> & message)
	{
		//char net = message->GetNet();
		message->SetNet(rpc::net::tcp);
		//message->GetHead().Add("n", net);
		message->SetSource(rpc::source::client);
		const RpcMethodConfig* methodConfig = nullptr;
		unsigned short opcode = message->GetOpcode();
		assert(message->GetHead().Has(rpc::Header::client_sock_id));
		if(opcode > 0)
		{
			methodConfig = RpcConfig::Inst()->GetMethodConfig(opcode);
			if(methodConfig == nullptr)
			{
				LOG_ERROR("not find rpc config : opcode:{}", opcode);
				return XCode::CallFunctionNotExist;
			}
			std::string func = fmt::format("{}.{}",
				methodConfig->service, methodConfig->method);
			message->GetHead().Add(rpc::Header::func, func);
		}
		else
		{
			const std::string& fullName = message->GetHead().GetStr(rpc::Header::func);
			methodConfig = RpcConfig::Inst()->GetMethodConfig(fullName);
			if(methodConfig == nullptr)
			{
				LOG_ERROR("not find rpc config : {}", fullName);
				return XCode::CallFunctionNotExist;
			}
		}
		if (!methodConfig->client || !methodConfig->open)
		{
			LOG_ERROR("call function not exist : {}.{}", methodConfig->service, methodConfig->method)
			return XCode::CallFunctionNotExist;
		}
		long long playerId = 0;
		message->SetNet(rpc::net::tcp);
		int serverId = this->mApp->GetNodeId();
		message->GetHead().Get(rpc::Header::id, playerId);
		Player* player = this->mPlayerComponent->Get(playerId);
		if (player == nullptr)
		{
			if(methodConfig->auth)
			{
				return XCode::CloseSocket;
			}
			this->mRouter->Send(serverId, message);
			return XCode::Ok;
		}
		const std::string& name = methodConfig->node;
		NodeCluster * nodeCluster = this->mNode->GetCluster(name);
		if(nodeCluster == nullptr)
		{
			return XCode::NotFoundActor;
		}
		switch (methodConfig->forward)
		{
			case rpc::forward::fixed: //转发到固定机器
				if (!player->GetNodeID(name, serverId))
				{
					return XCode::NotFoundServerRpcAddress;
				}
				break;
			case rpc::forward::random:
			{
				if(!nodeCluster->Random(serverId))
				{
					return XCode::NotFoundActor;
				}
				break;
			}
			case rpc::forward::hash:
			{
				if(!nodeCluster->Hash(playerId, serverId))
				{
					return XCode::NotFoundActor;
				}
				break;
			}
			case rpc::forward::next:
			{
				if(!nodeCluster->Next(serverId))
				{
					return XCode::NotFoundActor;
				}
				break;
			}
			default:
			LOG_ERROR("unknown forward {}", message->ToString());
				return XCode::Failure;
		}
		this->mRouter->Send(serverId, message);
		return XCode::Ok;
	}

	int GateComponent::OnResponse(std::unique_ptr<rpc::Message> & message)
	{
		int socketId, netType = 0;
		rpc::Head & head = message->GetHead();
		if (!head.Del("n", netType))
		{
			return XCode::UnKnowPacket;
		}
		if (!head.Del(rpc::Header::client_sock_id, socketId))
		{
			return XCode::UnKnowPacket;
		}
		message->SetNet((char)netType);
		message->SetSource(rpc::source::server);

		this->Send(socketId, message);
		return XCode::Ok;
	}
}