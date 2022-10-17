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
		XCode SaveAddress(long long userId, const s2s::allot::save &request);
        XCode AllotUser(const com::type::int64 & userId, s2s::allot::response & response);
		XCode QueryAddress(long long userId, const com::type::string &request, com::type::string & response); //查询玩家服务地址
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
