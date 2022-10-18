﻿#include"InnerNetMessageComponent.h"
#include"Component/TaskComponent.h"
#include"App/App.h"
#include"Lua/LuaServiceMethod.h"
#include"Config/ServiceConfig.h"
#include"InnerNetComponent.h"
#include"Timer/ElapsedTimer.h"
#include"Service/LocalRpcService.h"
#include"Async/RpcTaskSource.h"
namespace Sentry
{
	bool InnerNetMessageComponent::Awake()
	{
		this->mTaskComponent = nullptr;
		this->mTimerComponent = nullptr;
		this->mRpcClientComponent = nullptr;
        return true;
	}

	bool InnerNetMessageComponent::LateAwake()
	{
		this->mTaskComponent = this->mApp->GetTaskComponent();
		this->mTimerComponent = this->GetComponent<TimerComponent>();
		LOG_CHECK_RET_FALSE(this->mTaskComponent = this->GetComponent<TaskComponent>());
		LOG_CHECK_RET_FALSE(this->mRpcClientComponent = this->GetComponent<InnerNetComponent>());
		return true;
	}

	XCode InnerNetMessageComponent::OnRequest(std::shared_ptr<Rpc::Packet> message)
	{
        const Rpc::Head & head = message->GetHead();
        LOG_RPC_CHECK_ARGS(head.Get("func", this->mFullName));
        const RpcMethodConfig * methodConfig = RpcConfig::Inst()->GetMethodConfig(this->mFullName);
        if(methodConfig == nullptr)
        {
            return XCode::CallFunctionNotExist;
        }
        if(!methodConfig->Request.empty() && message->GetSize() == 0)
        {
            return XCode::CallArgsError;
        }

		RpcService * logicService = this->mApp->GetService(methodConfig->Service);
		if (logicService == nullptr || !logicService->IsStartService())
		{
            LOG_ERROR("call service not exist : [" << methodConfig->Service << "]");
			return XCode::CallServiceNotFound;
		}
        this->mWaitMessages.push(std::move(message));
		return XCode::Successful;
	}

    void InnerNetMessageComponent::OnSystemUpdate()
    {
        size_t count = 0;
        while(!this->mWaitMessages.empty() && count <= MAX_HANDLER_MSG_COUNT)
        {
            count++;
            std::shared_ptr<Rpc::Packet> message = this->mWaitMessages.front();
            if(message->GetHead().Get("func", this->mFullName))
            {
               const RpcMethodConfig * methodConfig = RpcConfig::Inst()->GetMethodConfig(this->mFullName);
               if(methodConfig != nullptr)
               {
                   if (!methodConfig->IsAsync)
                   {
                       this->Invoke(methodConfig, message);
                   }
                   this->mTaskComponent->Start(&InnerNetMessageComponent::Invoke, this, methodConfig, message);
               }
            }
            this->mWaitMessages.pop();
        }
    }

    void InnerNetMessageComponent::Invoke(const RpcMethodConfig *config, std::shared_ptr<Rpc::Packet> message)
    {
        XCode code = XCode::Failure;
        RpcService *logicService = this->mApp->GetService(config->Service);
        try
        {
            code = logicService->Invoke(config->Method, message);
        }
        catch (std::exception &e)
        {
            code = XCode::ThrowError;
            message->GetHead().Add("error", e.what());
        }
        long long rpcId = 0;
        if (!message->GetHead().Get("rpc", rpcId))
        {
            return; //不需要返回
        }
        std::string address;
        message->GetHead().Add("code", code);
        if (message->GetHead().Get("address", address))
        {
            message->GetHead().Remove("id");
            message->GetHead().Remove("address");
#ifndef __DEBUG__
            message->GetHead().Remove("func");
#endif
            message->SetType(Tcp::Type::Response);
            this->mRpcClientComponent->Send(address, message);
            return;
        }
    }

    bool InnerNetMessageComponent::Ping(const std::string &address)
    {
        std::shared_ptr<Rpc::Packet> message = std::make_shared<Rpc::Packet>();
        {
            message->SetType(Tcp::Type::Ping);
        }
        std::shared_ptr<Rpc::Packet> response = this->Call(address, message);
        return response->GetCode(XCode::Failure) == XCode::Successful;
    }

    std::shared_ptr<Rpc::Packet> InnerNetMessageComponent::Call(
        const std::string &address, std::shared_ptr<Rpc::Packet> message)
    {
        message->GetHead().Remove("address");
        assert(message->GetType() < (int)Tcp::Type::Max);
        assert(message->GetType() > (int)Tcp::Type::None);
        if (!this->mRpcClientComponent->Send(address, message))
        {
            return nullptr;
        }
        std::shared_ptr<RpcTaskSource> taskSource = std::make_shared<RpcTaskSource>(0);
        return this->AddTask(taskSource)->Await();
    }

    bool InnerNetMessageComponent::Send(const std::string &address, std::shared_ptr<Rpc::Packet> message)
    {
        message->GetHead().Remove("address");
        assert(message->GetType() < (int)Tcp::Type::Max);
        assert(message->GetType() > (int)Tcp::Type::None);
        return this->mRpcClientComponent->Send(address, message);
    }
}// namespace Sentry
