//
// Created by zmhy0073 on 2022/8/29.
//
#include"ServerWeb.h"
#include"Message/s2s/s2s.pb.h"
#include"Core/System/System.h"
#include"Entity/Unit/App.h"
#include"Server/Config/CodeConfig.h"
#include"Common/Service/Node.h"
#include"Util/File/DirectoryHelper.h"
#include"Util/File/FileHelper.h"
#include"Rpc/Component/LocationComponent.h"
#include"Registry/Component/RegistryComponent.h"
namespace Tendo
{
    bool ServerWeb::OnInit()
	{
		BIND_COMMON_HTTP_METHOD(ServerWeb::Info);
		BIND_COMMON_HTTP_METHOD(ServerWeb::Stop);
		BIND_COMMON_HTTP_METHOD(ServerWeb::Login);
		BIND_COMMON_HTTP_METHOD(ServerWeb::Hotfix);
		BIND_COMMON_HTTP_METHOD(ServerWeb::Register);
		LOG_CHECK_RET_FALSE(this->GetComponent<LocationComponent>());
		LOG_CHECK_RET_FALSE(this->GetComponent<RegistryComponent>());
		return true;
	}

    int ServerWeb::Login(const Http::Request &request, Http::DataResponse &response)
    {
        std::string account,password;
        Http::Parameter parameter(request.Content());
		LOG_ERROR_CHECK_ARGS(parameter.Get("account,", account));
		LOG_ERROR_CHECK_ARGS(parameter.Get("password", password));
        return XCode::Successful;
    }

    int ServerWeb::Register(const Http::Request& request, Http::DataResponse& response)
    {
        return XCode::Successful;
    }

	int ServerWeb::Hotfix(Json::Writer&response)
	{
		RpcService * rpcService = this->mApp->GetService<Node>();
		LocationComponent * locationComponent = this->GetComponent<LocationComponent>();
		if(locationComponent == nullptr || rpcService == nullptr)
		{
			response.Add("error").Add("LocationComponent or InnerService Is Null");
			return XCode::Failure;
		}
		// TODO

		return XCode::Successful;
	}

	int ServerWeb::Stop(Json::Writer & response)
	{
		std::string server;
		const ServerConfig * config = ServerConfig::Inst();
		RpcService * rpcService = this->mApp->GetService<Node>();
		RegistryComponent * pRegistryComponent = this->GetComponent<RegistryComponent>();
		LocationComponent * pLocationComponent = this->GetComponent<LocationComponent>();

		pRegistryComponent->Query(server);
		std::vector<LocationUnit *> servers;
		pLocationComponent->GetAllServer(servers);
		for(LocationUnit * locationUnit : servers)
		{
			if(locationUnit->GetId() != config->ServerId())
			{
				std::string address;
				locationUnit->Get("rpc", address);
				int code = rpcService->Call(address, "Stop");
				response.Add(address).Add(CodeConfig::Inst()->GetDesc(code));
			}
		}
		this->mApp->GetCoroutine()->Start(&App::Stop, this->mApp, 0);
		return XCode::Successful;
	}

	int ServerWeb::Info(Json::Writer&response)
    {
		return XCode::Successful;
    }
}