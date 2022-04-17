
//
// Created by mac on 2021/11/28.
//

#ifndef GAMEKEEPER_PROTOPROXYCOMPONENT_H
#define GAMEKEEPER_PROTOPROXYCOMPONENT_H
#include"Component/Component.h"
#include"Async/TaskSource.h"
#include"Task/RequestTaskQueueSource.h"
namespace Sentry
{
	class GateComponent final : public Component,
						  public IClientRpc<c2s::Rpc_Request, c2s::Rpc_Response>
	{
	 public:
		GateComponent() = default;
		~GateComponent() final = default;
	 protected:
		bool LateAwake() final;
	 public:
		void OnConnect(long long sockId) final;
		XCode OnRequest(std::shared_ptr<c2s::Rpc_Request> request) final;
		XCode OnResponse(long long sockId, std::shared_ptr<c2s::Rpc_Response> response) final;
	 private:
		void OnClientRequest(long long sockId);
	 private:
		class GateService * mGateService;
		class RpcComponent* mRpcComponent;
		class TaskComponent * mTaskComponent;
		class GateClientComponent* mGateClientComponent;
		std::unordered_map<long long, long long> mClientMap;
		std::unordered_map<long long, std::shared_ptr<RequestTaskQueueSource>> mClientTasks;
	};
}


#endif //GAMEKEEPER_PROTOPROXYCOMPONENT_H
