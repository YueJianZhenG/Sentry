
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
	 public:
		void OnConnect(const std::string & address) final;
		XCode OnRequest(std::shared_ptr<c2s::Rpc_Request> request) final;
		bool AddUserToken(const std::string & token, long long userId);
		XCode OnResponse(const std::string & address, std::shared_ptr<c2s::Rpc_Response> response) final;

	private:
		void OnUserRequest(long long userId, std::shared_ptr<c2s::Rpc::Request> request);
	private:
		bool LateAwake() final;
		void OnSocketTimeout(const std::string & address);
		XCode Auth(const std::shared_ptr<c2s::Rpc::Request> request);
	 private:
		class TaskComponent * mTaskComponent;
		class TimerComponent * mTimerComponent;
		class GateClientComponent* mGateClientComponent;
		std::unordered_map<std::string, long long> mUserTokens;
		std::unordered_map<std::string, unsigned int> mSocketTimers;
	};
}


#endif //GAMEKEEPER_PROTOPROXYCOMPONENT_H
