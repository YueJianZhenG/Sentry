//
// Created by zmhy0073 on 2022/10/13.
//

#ifndef APP_USERBEHAVIOR_H
#define APP_USERBEHAVIOR_H

#include"Message/s2s/s2s.pb.h"
#include"Rpc/Service/RpcService.h"
namespace acs
{
    class LoginSystem final : public RpcService
    {
    public:
        LoginSystem();
        ~LoginSystem() override = default;
    private:
		int Login(long long id, const s2s::login::request & request);
		int Logout(long long id, const s2s::logout::request & request);
	private:
		bool OnInit() final;
	private:
		class PlayerComponent * mPlayerMgr;
		class EventProxyComponent * mEventProxy;
	};
}


#endif //APP_USERBEHAVIOR_H
