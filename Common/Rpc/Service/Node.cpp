//
// Created by zmhy0073 on 2022/10/8.
//

#include"Node.h"
#include"Config/ClusterConfig.h"
#include"Component/InnerNetComponent.h"
#include"Component/NodeMgrComponent.h"
#include"Component/TextConfigComponent.h"
#include"Component/InnerNetMessageComponent.h"

namespace Sentry
{
    void Node::Init()
    {
        this->mApp->AddComponent<InnerNetComponent>();
        this->mApp->AddComponent<InnerNetMessageComponent>();
    }

    bool Node::OnStart()
    {
        BIND_COMMON_RPC_METHOD(Node::Ping);
        BIND_COMMON_RPC_METHOD(Node::Join);
        BIND_COMMON_RPC_METHOD(Node::Exit);
        BIND_COMMON_RPC_METHOD(Node::Stop);
        BIND_COMMON_RPC_METHOD(Node::Hotfix);
		BIND_COMMON_RPC_METHOD(Node::RunInfo);
		BIND_COMMON_RPC_METHOD(Node::LoadConfig);
        BIND_COMMON_RPC_METHOD(Node::AddAddress);
        BIND_COMMON_RPC_METHOD(Node::DelAddress);
		this->mNodeComponent = this->GetComponent<NodeMgrComponent>();
        return true;
    }

	int Node::Ping(const Rpc::Head& head)
    {
        std::string address;
        head.Get("address", address);
        CONSOLE_LOG_FATAL("[" << address << "] ping server");
        return XCode::Successful;
    }

    int Node::Join(const s2s::server::info &request)
	{
		const std::string & rpc = request.rpc();
		const std::string & http = request.http();
		const std::string & name = request.name();
		if (!ClusterConfig::Inst()->GetConfig(name))
		{
			LOG_ERROR("not find cluster config : " << name);
			return XCode::Failure;
		}
		this->mNodeComponent->AddRpcServer(name, rpc);
		this->mNodeComponent->AddHttpServer(name, http);
		return XCode::Successful;
	}

    int Node::Exit(const s2s::server::info &response)
    {
        return XCode::Successful;
    }

    int Node::Stop()
    {
        std::vector<RpcService *> components;
        if(this->mApp->GetComponents(components))
        {
            for (RpcService *component: components)
            {
                if(component->IsStartService())
                {
                    component->WaitAllMessageComplete();
                    component->Close();
                }
            }
        }

		CONSOLE_LOG_INFO("shutdown server int 10s after");
		TimerComponent * timerComponent = this->mApp->GetTimerComponent();
		timerComponent->DelayCall(10 * 1000, &App::Stop, this->mApp);
        return XCode::Successful;
    }

    int Node::LoadConfig()
    {
        TextConfigComponent * textComponent = this->GetComponent<TextConfigComponent>();
        if(textComponent != nullptr)
        {
            textComponent->OnHotFix();
        }
        return XCode::Successful;
    }

	int Node::RunInfo(com::type::string& response)
	{
		Json::Writer document;
        {
            document.BeginObject("server");
            document.Add("name").Add(ServerConfig::Inst()->Name());
            document.Add("fps").Add((int)this->mApp->GetFps());
            document.Add("cpu").Add(std::thread::hardware_concurrency());
            document.Add(Json::End::EndObject);
        }
        std::vector<Component *> components;
        this->mApp->GetComponents(components);
		for(Component * component : components)
		{
            const char* key = component->GetName().c_str();
			IServerRecord * serverRecord = component->Cast<IServerRecord>();
			if(serverRecord != nullptr)
			{
				IServiceBase * serviceBase = component->Cast<IServiceBase>();
				if(serviceBase != nullptr && !serviceBase->IsStartService())
				{
					continue;
				}
                document.BeginObject(key);
                serverRecord->OnRecord(document);
                document.Add(Json::End::EndObject);				
			}
		}
		document.WriterStream(response.mutable_str());
		return XCode::Successful;
	}

    int Node::AddAddress(long long userId, const s2s::server::list& request)
    {
        for (int index = 0; index < request.list_size(); index++)
        {
            const s2s::server::info& info = request.list(index);
            this->mNodeComponent->AddRpcServer(info.name(), userId, info.rpc());
#ifdef __DEBUG__
            CONSOLE_LOG_DEBUG("user " << userId << " add server [" << info.rpc() << "]");
#endif
        }
        return XCode::Successful;
    }

    int Node::DelAddress(long long userId, const com::array::string& request)
    {
        for (int index = 0; index < request.array_size(); index++)
        {
            const std::string & name = request.array(index);
            this->mNodeComponent->DelServer(name, userId);
        }
        return XCode::Successful;
    }

    int Node::Hotfix()
    {
        std::vector<IHotfix *> components;
		this->mApp->GetComponents(components);
        for(IHotfix * component : components)
        {
            component->OnHotFix();
        }
        return XCode::Successful;
    }
}