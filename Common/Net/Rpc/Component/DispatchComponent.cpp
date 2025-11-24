#include "DispatchComponent.h"
#include "Rpc/Config/ServiceConfig.h"
#include "Timer/Timer/ElapsedTimer.h"
#ifdef __DEBUG__
#include "Proto/Component/ProtoComponent.h"
#endif
#include "Async/Component/CoroutineComponent.h"

#include "XCode/XCode.h"
#include "Entity/Actor/App.h"
#include "Rpc/Common/Message.h"
#include "Core/System/System.h"
#include "Rpc/Service/RpcService.h"
#include "Server/Config/CodeConfig.h"
#include "Router/Component/RouterComponent.h"

namespace acs
{
	DispatchComponent::DispatchComponent()
	{
		this->mSumCount = 0;
		this->mWaitCount = 0;
		this->mRouter = nullptr;
		this->mCoroutine = nullptr;
		this->mGateSender = nullptr;
	}

	bool DispatchComponent::LateAwake()
	{
		std::vector<RpcService*> rpcServices;
		this->mApp->GetComponents(rpcServices);
		for (RpcService* rpcService: rpcServices)
		{
			const std::string& name = rpcService->GetName();
			LOG_CHECK_RET_FALSE(this->mRpcServices.Add(name, rpcService));
		}
		this->mGateSender = this->mApp->GetComponent<rpc::IOuterSender>();
		LOG_CHECK_RET_FALSE(this->mRouter = this->GetComponent<RouterComponent>())
		LOG_CHECK_RET_FALSE(this->mCoroutine = this->GetComponent<CoroutineComponent>())
		return true;
	}

	void DispatchComponent::OnAppStop()
	{
		while (this->mWaitCount > 1)
		{
			this->mApp->Sleep();
			LOG_INFO("wait message count:{}", this->mWaitCount);
		}
	}

	int DispatchComponent::OnRequest(std::unique_ptr<rpc::Message> & message)
	{
		++this->mSumCount;
		int code = XCode::Ok;
		std::string fullName;
		const RpcMethodConfig* methodConfig = nullptr;
		const rpc::Head & parameter = message->ConstHead();
		if(message->MsgType() == rpc::msg::opcode)
		{
			unsigned short opcode = message->GetOpcode();
			methodConfig = RpcConfig::Inst()->GetMethodConfig(opcode);
			if(methodConfig != nullptr)
			{
				fullName = methodConfig->fullname;
			}
		}
		else
		{
			parameter.Get(rpc::Header::func, fullName);
			methodConfig = RpcConfig::Inst()->GetMethodConfig(fullName);
		}
		do
		{
			if (methodConfig == nullptr)
			{
				LOG_ERROR("not find rpc method => {}", fullName)
				code = XCode::CallFunctionNotExist;
				break;
			}
			if(!methodConfig->open)
			{
				code = XCode::CallFunctionNotExist;
				break;
			}
			if(message->GetSource() == rpc::source::client)
			{
				if(!methodConfig->client)
				{
					code = XCode::PermissionDenied;
					break;
				}
				if(!parameter.Has(rpc::Header::id))
				{
					code = XCode::PermissionDenied;
					break;
				}
			}

			//LOG_DEBUG("call ({}) by {}", fullName, methodConfig->NetName);
			if (!this->mRpcServices.Has(methodConfig->service))
			{
				code = XCode::CallServiceNotFound;
				break;
			}
			if (!methodConfig->async)
			{
				this->Invoke(methodConfig, message);
				break;
			}
			this->mCoroutine->Start(&DispatchComponent::Invoke, this, methodConfig, std::move(message));
			return XCode::Ok;
		}
		while (false);
		if(code != XCode::Ok)
		{
#ifdef __DEBUG__
			const std::string & desc = CodeConfig::Inst()->GetDesc(code);
			LOG_DEBUG("[{}] => {}", fullName, desc);
#endif
			int id = message->SockId();
			this->mRouter->Send(id, code, message);
		}
		return code;
	}

	void DispatchComponent::Invoke(const RpcMethodConfig* config, std::unique_ptr<rpc::Message> & message)
	{
		++this->mWaitCount;
		int code = XCode::Ok;
#ifdef __DEBUG__
		long long start = help::Time::NowMil();
#endif
		do
		{
			const std::string& service = config->service;
			RpcService* logicService = this->mRpcServices.Find(service);
			if (logicService == nullptr)
			{
				code = XCode::CallServiceNotFound;
				LOG_ERROR("call {} server not found", config->fullname);
				break;
			}
			try
			{
				code = logicService->Invoke(config, message);
			}
			catch(const std::exception & e)
			{
				code = XCode::ThrowError;
				message->SetError(e.what());
				LOG_ERROR("call rpc [{}] => {}", config->fullname, e.what());
			}
			catch(...)
			{
				code = XCode::ThrowError;
				message->SetError("unknown exception error");
				LOG_ERROR("call rpc [{}] => unknown exception error", config->fullname);
			}
		} while (false);
#ifdef __DEBUG__
		long long t = help::Time::NowMil() - start;
		//if (code != XCode::Ok)
		{
			const std::string& desc = CodeConfig::Inst()->GetDesc(code);
			LOG_BY_NAME("rpc", "({}ms) call rpc [{}] code:{} = {}", t, config->fullname, code, desc);
		}
#endif
		--this->mWaitCount;
		this->mRouter->Send(message->SockId(), code, message);
	}

	void DispatchComponent::OnRecord(json::w::Document& document)
	{
		std::unique_ptr<json::w::Value> data = document.AddObject("dispatch");
		{
			data->Add("sum", this->mSumCount);
			data->Add("wait", this->mWaitCount);
			data->Add("rpc_wait", this->AwaitCount());
		}
	}

	int DispatchComponent::OnMessage(std::unique_ptr<rpc::Message> & message)
	{

		switch (message->GetType())
		{
			case rpc::type::request:
				return this->OnRequest(message);
			case rpc::type::response:
			{
				if(message->GetSource() == rpc::source::client)
				{
					if (this->mGateSender != nullptr)
					{
						int socketId = 0;
						rpc::Head & parameter = message->GetHead();
						if(parameter.Get(rpc::Header::sock_id, socketId))
						{
							this->mGateSender->Send(socketId, message);
						}
						return XCode::Ok;
					}
				}
				int rpcId = message->GetRpcId();
				this->OnResponse(rpcId, message);
				return XCode::Ok;
			}
			case rpc::type::client:
				return this->OnClient(message);
			case rpc::type::broadcast:
				return this->OnBroadcast(message);
		}
		LOG_ERROR("unknown message type : {}", message->GetType());
		return XCode::UnKnowPacket;
	}

	int DispatchComponent::OnClient(std::unique_ptr<rpc::Message> & message)
	{
		if (this->mGateSender == nullptr)
		{
			return XCode::NetWorkError;
		}
		int sockId = 0;
		if (!message->GetHead().Get(rpc::Header::client_sock_id, sockId))
		{
			LOG_ERROR("not find userid forward to client fail");
			return XCode::CallArgsError;
		}
		message->SetType(rpc::type::request);
		message->GetHead().Del(rpc::Header::client_sock_id);
		this->mGateSender->Send(sockId, message);
		return XCode::Ok;
	}

	int DispatchComponent::OnBroadcast(std::unique_ptr<rpc::Message> & message)
	{
		if (this->mGateSender == nullptr)
		{
			return XCode::Failure;
		}
		message->SetType(rpc::type::request);
		this->mGateSender->Broadcast(message);
		return XCode::Ok;
	}
}// namespace Sentry
