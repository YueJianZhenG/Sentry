//
// Created by zmhy0073 on 2021/12/1.
//

#ifndef GAMEKEEPER_GATESERVICE_H
#define GAMEKEEPER_GATESERVICE_H
#include"Service/LocalRpcService.h"
namespace Sentry
{
	class OuterService final : public LocalRpcService
	{
	 public:
		OuterService() = default;
		~OuterService() final = default;
	 private:
		XCode Ping(long long userId);
        XCode AllotUser(const com::type::int64 & userId, s2s::allot::response & response);
	private:
		bool Awake() final;
		bool OnStart() final;
        bool OnClose() final;
	 private:
		std::string mAddress;
		class TimerComponent* mTimerComponent;
		class OuterNetMessageComponent* mOuterComponent;
	};

}

#endif //GAMEKEEPER_GATESERVICE_H
