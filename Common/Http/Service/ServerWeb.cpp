//
// Created by zmhy0073 on 2022/8/29.
//
#include"ServerWeb.h"
#include"Core/System/System.h"
#include"Entity/App/App.h"
#include"Server/Config/CodeConfig.h"
#include"Common/Service/Node.h"
#include"Registry/Service/Registry.h"
#include"Util/File/DirectoryHelper.h"
#include"Util/File/FileHelper.h"
#include"Rpc/Component/NodeMgrComponent.h"
namespace Sentry
{
    bool ServerWeb::OnInit()
	{
		BIND_COMMON_HTTP_METHOD(ServerWeb::Info);
		BIND_COMMON_HTTP_METHOD(ServerWeb::Stop);
		BIND_COMMON_HTTP_METHOD(ServerWeb::Login);
		BIND_COMMON_HTTP_METHOD(ServerWeb::Hotfix);
		BIND_COMMON_HTTP_METHOD(ServerWeb::Register);
		return true;
	}

    int ServerWeb::Login(const Http::Request &request, Http::DataResponse &response)
    {
        std::string account,password;
        Http::Parameter parameter(request.Content());
        LOG_ERROR_RETURN_CODE(parameter.Get("account,", account), XCode::CallArgsError);
        LOG_ERROR_RETURN_CODE(parameter.Get("password", password), XCode::CallArgsError);
        return XCode::Successful;
    }

    int ServerWeb::Register(const Http::Request& request, Http::DataResponse& response)
    {
        return XCode::Successful;
    }

	int ServerWeb::Hotfix(Json::Writer&response)
	{
		RpcService * rpcService = this->mApp->GetService<Node>();
		NodeMgrComponent * locationComponent = this->GetComponent<NodeMgrComponent>();
		if(locationComponent == nullptr || rpcService == nullptr)
		{
			response.Add("error").Add("LocationComponent or InnerService Is Null");
			return XCode::Failure;
		}
		std::vector<std::string> locations;
		locationComponent->GetServers(locations);
		for(const std::string & location : locations)
		{
			int code = rpcService->Call(location, "Hotfix");
            response.Add(location.c_str()).Add(CodeConfig::Inst()->GetDesc(code));
		}
		return XCode::Successful;
	}

	int ServerWeb::Stop(Json::Writer & response)
	{
		std::string address;
		Node * node = this->GetComponent<Node>();
		RpcService* rpcService = this->mApp->GetService<Registry>();
		NodeMgrComponent * locationComponent = this->GetComponent<NodeMgrComponent>();
		if(!locationComponent->GetServer(rpcService->GetServer(), address))
		{
			response.Add("error").Add("not find registry server address");
			return XCode::AddressAllotFailure;
		}
		com::type::string message;
		std::shared_ptr<s2s::server::list> list
			= std::make_shared<s2s::server::list>();
		int code = rpcService->Call(address, "Query", message, list);
		if(code != XCode::Successful)
		{
			response.Add("error").Add(CodeConfig::Inst()->GetDesc(code));
			return XCode::Failure;
		}
		for (int index = 0; index < list->list_size(); index++)
		{
			const s2s::server::info& info = list->list(index);
			{
				int code = node->Call(info.rpc(), "Stop");
				response.Add(info.rpc()).Add(CodeConfig::Inst()->GetDesc(code));
			}
		}
		return XCode::Successful;
	}

	int ServerWeb::Info(Json::Writer&response)
    {
		std::string address;
		Node * node = this->GetComponent<Node>();
		RpcService* rpcService = this->mApp->GetService<Registry>();
		NodeMgrComponent * locationComponent = this->GetComponent<NodeMgrComponent>();
		if(!locationComponent->GetServer(rpcService->GetServer(), address))
		{
			response.Add("error").Add("not find registry server address");
			return XCode::AddressAllotFailure;
		}

		com::type::string message;
		const std::string func("Query");
		std::shared_ptr<s2s::server::list> list = std::make_shared<s2s::server::list>();
		int code = rpcService->Call(address, func, message, list);
		if(code != XCode::Successful)
		{
			response.Add("error").Add(CodeConfig::Inst()->GetDesc(code));
			return XCode::Failure;
		}
		const std::string method("RunInfo");
		for (int index = 0; index < list->list_size(); index++)
		{
			const s2s::server::info& info = list->list(index);
			std::shared_ptr<com::type::string> resp =
				std::make_shared<com::type::string>();
			int code = node->Call(info.rpc(), method, resp);
			const std::string& desc = CodeConfig::Inst()->GetDesc(code);
			if (code == XCode::Successful)
			{
				rapidjson::Document document;
				const std::string& json = resp->str();
				if (!document.Parse(json.c_str(), json.size()).HasParseError())
				{					
					response.Add(info.rpc()).Add(document);
				}
			}
			else
			{
				response.Add(info.rpc()).Add(desc);
			}
		}
		return XCode::Successful;
    }

}