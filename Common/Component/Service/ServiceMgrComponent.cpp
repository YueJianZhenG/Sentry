﻿#include "ServiceMgrComponent.h"
#include <Service/LocalServiceComponent.h>
#include <Coroutine/CoroutineComponent.h>
#include <Util/StringHelper.h>
#include <Core/App.h>
#include <Util/JsonHelper.h>
#include <Scene/ProtocolComponent.h>
#include <Pool/MessagePool.h>
#include <Method/LuaServiceMethod.h>
#include <Network/Tcp/TcpClientComponent.h>
namespace GameKeeper
{
    bool ServiceMgrComponent::Awake()
    {
		ServerConfig & ServerCfg = App::Get().GetConfig();
		this->mCorComponent = App::Get().GetCorComponent();
        GKAssertRetFalse_F(ServerCfg.GetValue("NodeId", this->mNodeId));

        GKAssertRetFalse_F(this->mCorComponent = this->GetComponent<CoroutineComponent>());
        GKAssertRetFalse_F(this->mProtocolComponent = this->GetComponent<ProtocolComponent>());
		GKAssertRetFalse_F(this->mNetSessionComponent = this->GetComponent<TcpClientComponent>());
        return true;
    }

    bool ServiceMgrComponent::OnRequestMessage(const com::DataPacket_Request & request)
    {
        unsigned short methodId = request.methodid();
        const ProtocolConfig *protocolConfig = this->mProtocolComponent->GetProtocolConfig(methodId);
        if (protocolConfig == nullptr)
        {
            return false;
        }
        const std::string &service = protocolConfig->ServiceName;
        auto logicService = this->gameObject->GetComponent<ServiceComponent>(service);
        if (logicService == nullptr)
        {
            GKDebugFatal("call service not exist : [" << service << "]");
            return false;
        }

        const std::string &methodName = protocolConfig->Method;
        ServiceMethod *method = logicService->GetMethod(methodName);
        if (service == nullptr)
        {
            GKDebugFatal("call method not exist : [" << service << "." << methodName << "]");
            return false;
        }

        if (!protocolConfig->IsAsync)
        {
            std::string responseContent;
            //method->SetAddress(request.address()); //TODO
            XCode code = method->Invoke(request, responseContent);
            if (request.rpcid() != 0)
            {
                this->mResponse.Clear();
                this->mResponse.set_code(code);
                this->mResponse.set_rpcid(request.rpcid());
                this->mResponse.set_userid(request.userid());
                this->mResponse.set_methodid(request.methodid());
                this->mResponse.set_messagedata(responseContent);
                this->mNetSessionComponent->SendByAddress(request.socketid(), this->mResponse);
            }
        }
        else if(method->IsLuaMethod()) //lua 异步
        {

        }
        else
        {
			com::DataPacket_Request * requestData = this->mRequestDataPool.CopyFrom(request);
            this->mCorComponent->StartCoroutine(&ServiceMgrComponent::Invoke, this, method, requestData);
        }
        return true;
    }

	void ServiceMgrComponent::Invoke(ServiceMethod * method, com::DataPacket_Request * request)
    {
        std::string responseContent;
        //method->SetAddress(request->address());
        XCode code = method->Invoke(*request, responseContent);
        if (request->rpcid() != 0)
        {
            this->mResponse.Clear();
            this->mResponse.set_code(code);
            this->mResponse.set_rpcid(request->rpcid());
            this->mResponse.set_userid(request->userid());
            this->mResponse.set_methodid(request->methodid());
            this->mResponse.set_messagedata(responseContent);
            this->mNetSessionComponent->SendByAddress(request->socketid(), this->mResponse);

			this->mRequestDataPool.Destory(request);
        }
    }
}// namespace GameKeeper
