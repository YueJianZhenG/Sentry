//
// Created by zmhy0073 on 2021/12/1.
//

#include"GateService.h"
#include"App/App.h"
#include"Util/MD5.h"
#include"Util/Guid.h"
#include"NetWork/RpcGateClient.h"
#include"Component/Logic/UserSubService.h"
#include"Component/Scene/EntityMgrComponent.h"
#include"Component/Gate/GateClientComponent.h"

namespace Sentry
{
	bool GateService::OnInitService(ServiceMethodRegister& methodRegister)
	{
		methodRegister.Bind("Ping", &GateService::Ping);
		methodRegister.Bind("Login", &GateService::Login);
		methodRegister.Bind("Allot", &GateService::Allot);
		this->mGateComponent = this->GetComponent<GateClientComponent>();
		this->mEntityComponent = this->GetComponent<EntityMgrComponent>();
		if (this->mGateComponent == nullptr)
		{
			this->mGateComponent = this->mEntity->GetOrAddComponent<GateClientComponent>();
			LOG_CHECK_RET_FALSE(this->mGateComponent->LateAwake());
		}

		if (this->mEntityComponent == nullptr)
		{
			this->mEntityComponent = this->mEntity->GetOrAddComponent<EntityMgrComponent>();
			LOG_CHECK_RET_FALSE(this->mEntityComponent->LateAwake());
		}
		return true;
	}

	bool GateService::LateAwake()
	{
		LOG_CHECK_RET_FALSE(LocalServerRpc::LateAwake());
		LOG_CHECK_RET_FALSE(this->GetComponent<GateClientComponent>());
		this->mRpcAddress = this->GetApp()->GetConfig().GetRpcAddress();
		LOG_CHECK_RET_FALSE(this->mUserService = this->GetComponent<UserSubService>());
		LOG_CHECK_RET_FALSE(this->mTimerComponent = this->GetComponent<TimerComponent>());
		return true;
	}

	XCode GateService::Ping()
	{
		return XCode::Failure;
	}

	XCode GateService::Login(const c2s::GateLogin::Request& request)
	{

		return XCode::Successful;
	}

	XCode GateService::Allot(const s2s::AddressAllot::Request& request)
	{
		long long userId = request.user_id();
		const std::string& token = request.login_token();
		auto iter = this->mTokenMap.find(token);
		if (iter != this->mTokenMap.end())
		{
			this->mTokenMap.erase(iter);
		}
		Json::Writer jsonWriter;
		jsonWriter.AddMember("user_id", userId);
		jsonWriter.AddMember("service", "GateService");
		jsonWriter.AddMember("address", this->mRpcAddress);
		if (this->mUserService->Publish("AddUser", jsonWriter))
		{
			this->mTokenMap.emplace(token, userId);
			return XCode::Successful;
		}
		return XCode::AddressAllotFailure;
	}

	void GateService::OnTokenTimeout(const std::string& token)
	{
		auto iter = this->mTokenMap.find(token);
		if (iter != this->mTokenMap.end())
		{
			this->mTokenMap.erase(iter);
		}
	}
}