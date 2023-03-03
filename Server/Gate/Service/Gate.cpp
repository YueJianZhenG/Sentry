//
// Created by zmhy0073 on 2021/12/1.
//

#include"Gate.h"
#include"App/App.h"
#include"Md5/MD5.h"
#include"Client/OuterNetClient.h"
#include"Component/OuterNetComponent.h"
#include"Component/GateHelperComponent.h"
#include"Component/OuterNetMessageComponent.h"
namespace Sentry
{
    Gate::Gate()
    {
        this->mOuterComponent = nullptr;
    }
    void Gate::Init()
    {
        this->mApp->AddComponent<OuterNetComponent>();
        this->mApp->AddComponent<OuterNetMessageComponent>();
    }

	bool Gate::OnStart()
	{
        BIND_COMMON_RPC_METHOD(Gate::Allot);
        const ServerConfig * config = ServerConfig::Inst();
        ServerConfig::Inst()->GetLocation("gate", this->mAddress);
        this->mOuterComponent = this->GetComponent<OuterNetMessageComponent>();
        LOG_CHECK_RET_FALSE(config->GetLocation("gate", this->mAddress));
        return true;
	}

    bool Gate::OnClose()
    {
		return this->GetComponent<OuterNetComponent>()->StopListen();
    }

	int Gate::Ping(long long userId)
	{
		LOG_ERROR(userId << " ping gate server");
		return XCode::Failure;
	}

	int Gate::Allot(const com::type::int64 &request, s2s::allot::response &response)
    {
        std::string token;
        long long userId = request.value();
        if(!this->mOuterComponent->CreateToken(userId, token))
        {
            return XCode::Failure;
        }
        response.set_token(token);
        response.set_address(this->mAddress);
        return XCode::Successful;
    }
}