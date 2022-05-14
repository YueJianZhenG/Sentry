//
// Created by zmhy0073 on 2021/12/1.
//

#ifndef GAMEKEEPER_GATESERVICE_H
#define GAMEKEEPER_GATESERVICE_H
#include"Component/RpcService/LocalServiceComponent.h"
namespace Sentry
{
	class GateService final : public LocalServiceComponent
	{
	 public:
		GateService() = default;
		~GateService() final = default;
	 public:
		XCode Auth(const c2s::GateAuth::Request & request);
	 private:
		XCode Ping(long long userId);
		XCode QueryAddress(com::Type::String & response);
		XCode CallClient(long long userId, c2s::Rpc::Call & request);
		XCode BroadCast(const s2s::GateBroadCast::Request & request);
	 private:
		bool LateAwake() final;
		bool OnInitEvent(ServiceEventRegister &methodRegister) final;
		bool OnInitService(ServiceMethodRegister & methodRegister) final;
	 private:
		std::string mAddress;
		class GateComponent * mGateComponent;
		class TimerComponent* mTimerComponent;
		class MainRedisComponent * mRedisComponent;
		class GateClientComponent* mGateClientComponent;
	};

}

#endif //GAMEKEEPER_GATESERVICE_H
