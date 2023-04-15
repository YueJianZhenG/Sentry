//
// Created by zmhy0073 on 2022/10/25.
//

#include"Registry.h"

#include"Entity/Unit/App.h"
#include"Common/Service/Node.h"
#include"Util/String/StringHelper.h"
#include"Cluster/Config/ClusterConfig.h"
#include"Rpc/Component/NodeMgrComponent.h"
#include"Proto/Component/ProtoComponent.h"
#include"Redis/Component/RedisComponent.h"
#include"Redis/Component/RedisLuaComponent.h"
#include"google/protobuf/util/json_util.h"
namespace Tendo
{
	Registry::Registry()
	{
		this->mNodeComponent = nullptr;
		this->mRedisComponent = nullptr;
	}

	bool Registry::Awake()
	{
		this->mApp->AddComponent<RedisComponent>();
		return true;
	}

	bool Registry::OnInit()
	{
		BIND_COMMON_RPC_METHOD(Registry::Ping);
		BIND_COMMON_RPC_METHOD(Registry::Query);
		BIND_ADDRESS_RPC_METHOD(Registry::Register);
		BIND_COMMON_RPC_METHOD(Registry::UnRegister);
		this->mRedisComponent = this->GetComponent<RedisLuaComponent>();
		this->mNodeComponent = this->GetComponent<NodeMgrComponent>();
		return true;
	}

	bool Registry::OnStart()
	{
		std::shared_ptr<Message> message;
		ProtoComponent* component = this->GetComponent<ProtoComponent>();
		LOG_CHECK_RET_FALSE(component->Import("mysql/server.proto"));
		if (!component->New("server.registry", message))
		{
			LOG_ERROR("create protobuf type [server.registry] error");
			return false;
		}
		return true;
	}

	int Registry::Query(const s2s::server::query& request, s2s::server::list& response)
	{
		std::vector<std::string> list;
		if (!request.server_name().empty())
		{
			const std::string& name = request.server_name();
			if (ClusterConfig::Inst()->GetConfig(name) == nullptr)
			{
				return XCode::NotFoundRpcConfig;
			}
			list.emplace_back(name);
		}
		else
		{		
			ClusterConfig::Inst()->GetServers(list);		
		}
		for (const std::string& server : list)
		{

		}
		return XCode::Successful;
	}

	int Registry::Register(const std::string & address, const s2s::server::info& request)
	{
		if(address.find("tcp") != 0)
		{
			return XCode::OnlyUseTcpProtocol;
		}
		std::string json;
		const std::string func("registry.add");
		LOG_ERROR_RETURN_CODE(!request.name().empty(), XCode::CallArgsError);
		LOG_ERROR_RETURN_CODE(!request.listens().empty(), XCode::CallArgsError);
		if(!util::MessageToJsonString(request, &json).ok())
		{
			return XCode::ProtoCastJsonFailure;
		}
		std::shared_ptr<RedisResponse> response =
				this->mRedisComponent->Call(func, json, false);
		LOG_ERROR_RETURN_CODE(response && response->GetNumber() >= 0, XCode::SaveToRedisFailure);

		this->mServers.emplace(address);
		RpcService * node = this->mApp->GetService<Node>();
		for(const std::string & address : this->mServers)
		{
			node->Send(address, "Join", request);
		}
		return XCode::Successful;
	}

	int Registry::Ping(const Rpc::Packet& packet)
	{
		
		return XCode::Successful;
	}

	int Registry::UnRegister(const com::type::int32& request)
	{

		return XCode::Successful;
	}

	void Registry::OnSecondUpdate(int tick)
	{

	}
}