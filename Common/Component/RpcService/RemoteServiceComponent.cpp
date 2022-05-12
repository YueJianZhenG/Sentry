﻿#include"RemoteServiceComponent.h"
#include"App/App.h"
#include"Method/LuaServiceMethod.h"
#include"Global/ServiceConfig.h"
#ifdef __DEBUG__
#include"Pool/MessagePool.h"
#endif
#include"Util/StringHelper.h"
#include"Async/RpcTask/RpcTaskSource.h"
#include"Component/Rpc/RpcHandlerComponent.h"
#include"Component/Rpc/RpcClientComponent.h"
#include"Component/Redis/MainRedisComponent.h"

namespace Sentry
{
	bool RemoteServiceComponent::LateAwake()
	{
		this->mRpcComponent = this->GetComponent<RpcHandlerComponent>();
		this->mRedisComponent = this->GetComponent<MainRedisComponent>();
		this->mClientComponent = this->GetComponent<RpcClientComponent>();
		return this->GetConfig().GetListenerAddress("rpc", this->mLocalAddress);
	}

	std::shared_ptr<com::Rpc::Request> RemoteServiceComponent::NewRpcRequest(const std::string& func, long long userId, const Message* message)
	{
		const ServiceConfig& rpcConfig = this->GetApp()->GetServiceConfig();
		string name = fmt::format("{0}.{1}", this->GetName(), func);
		const RpcInterfaceConfig * protoConfig = rpcConfig.GetInterfaceConfig(name);
		if (protoConfig == nullptr)
		{
			LOG_ERROR("not find rpc config : " << name);
			return nullptr;
		}
		std::shared_ptr<com::Rpc::Request> request
			= std::make_shared<com::Rpc::Request>();

		request->set_user_id(userId);
		request->set_method_id(protoConfig->InterfaceId);
		if(message == nullptr)
		{
			return request;
		}
		request->mutable_data()->PackFrom(*message);
		return request;
	}
}

namespace Sentry
{
	XCode RemoteServiceComponent::Call(const std::string & address, const string& func)
	{
		std::shared_ptr<com::Rpc::Request> rpcRequest = this->NewRpcRequest(func, 0, nullptr);
		if(rpcRequest == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}
		std::shared_ptr<com::Rpc::Response> rpcResponse = this->StartCall(address, rpcRequest);
		return rpcResponse != nullptr ? (XCode)rpcResponse->code() : XCode::NetWorkError;
	}

	XCode RemoteServiceComponent::Call(const std::string & address, const string& func, const Message& message)
	{
		std::shared_ptr<com::Rpc::Request> rpcRequest = this->NewRpcRequest(func, 0, &message);
		if(rpcRequest == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}
		std::shared_ptr<com::Rpc::Response> rpcResponse = this->StartCall(address, rpcRequest);
		return rpcResponse != nullptr ? (XCode)rpcResponse->code() : XCode::NetWorkError;
	}

	XCode RemoteServiceComponent::Call(const std::string & address, const string& func, std::shared_ptr<Message> response)
	{
		assert(response != nullptr);
		std::shared_ptr<com::Rpc::Request> rpcRequest = this->NewRpcRequest(func, 0, nullptr);
		if(rpcRequest == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}
		std::shared_ptr<com::Rpc::Response> rpcResponse = this->StartCall(address, rpcRequest);
		if(rpcResponse == nullptr)
		{
			return XCode::NetWorkError;
		}
		if(rpcResponse->code() == (int)XCode::Successful && rpcResponse->has_data())
		{
			rpcResponse->mutable_data()->UnpackTo(response.get());
		}
		return (XCode)rpcResponse->code();
	}


	XCode RemoteServiceComponent::Call(const std::string & address, const string& func, const Message& message,
			std::shared_ptr<Message> response)
	{
		assert(response != nullptr);
		std::shared_ptr<com::Rpc::Request> rpcRequest = this->NewRpcRequest(func, 0, &message);
		if(rpcRequest == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}
		std::shared_ptr<com::Rpc::Response> rpcResponse = this->StartCall(address, rpcRequest);
		if(rpcResponse == nullptr)
		{
			return XCode::NetWorkError;
		}
		if(rpcResponse->code() == (int)XCode::Successful && rpcResponse->has_data())
		{
			rpcResponse->mutable_data()->UnpackTo(response.get());
		}
		return (XCode)rpcResponse->code();
	}

	XCode RemoteServiceComponent::PublishEvent(const std::string& eveId)
	{
		std::string data = "*";
		eve::Publish publishData;
		publishData.set_eve_id(eveId);
		if(!publishData.AppendToString(&data))
		{
			return XCode::SerializationFailure;
		}
		if(this->mRedisComponent->Publish(eveId, data) < 0)
		{
			return XCode::NetWorkError;
		}
		return XCode::Successful;
	}

	XCode RemoteServiceComponent::PublishEvent(const std::string& eveId, const Message& message)
	{
		std::string data = "*";
		eve::Publish publishData;
		publishData.set_eve_id(eveId);
		publishData.mutable_data()->PackFrom(message);
		if(!publishData.AppendToString(&data))
		{
			return XCode::SerializationFailure;
		}
		if(this->mRedisComponent->Publish(eveId, data) < 0)
		{
			return XCode::NetWorkError;
		}
		return XCode::Successful;
	}

	std::shared_ptr<com::Rpc::Response> RemoteServiceComponent::StartCall(const std::string & address,
			std::shared_ptr<com::Rpc::Request> request)
	{
		std::shared_ptr<RpcTaskSource> taskSource =
			std::make_shared<RpcTaskSource>();
		request->set_rpc_id(taskSource->GetRpcId());
		this->mRpcComponent->AddRpcTask(taskSource);
#ifdef __DEBUG__
		this->mRpcComponent->AddRpcInfo(taskSource->GetRpcId(), request->method_id());
#endif
		XCode code = this->SendRequest(address, request);
		if(code != XCode::Successful)
		{
			std::shared_ptr<com::Rpc::Response> response =
				std::make_shared<com::Rpc::Response>();
			response->set_code(int(code));
			return response;
		}
		return taskSource->Await();
	}

	XCode RemoteServiceComponent::SendRequest(const std::string& address, std::shared_ptr<com::Rpc::Request> request)
	{
		const ServiceConfig& serviceConfig = this->GetApp()->GetServiceConfig();
		const RpcInterfaceConfig * protoConfig = serviceConfig.GetInterfaceConfig(request->method_id());
		//LOG_INFO("start call " << protoConfig->FullName);
		if(protoConfig->CallWay == "Sub") //通过redis 的发布订阅发送
		{
			std::string message = "+";
			if(request->AppendToString(&message))
			{
				long long num = this->mRedisComponent->Publish(address, message);
				return num == 1 ? XCode::Successful : XCode::NetWorkError;
			}
			return XCode::SerializationFailure;
		}
		std::shared_ptr<ServerRpcClientContext> clientContext = this->mClientComponent->GetOrCreateSession(address);
		if(clientContext->GetSocketProxy()->IsOpen())
		{
			clientContext->SendToServer(request);
			return XCode::Successful;
		}
		int count = 0;
		while(!clientContext->StartConnectAsync())
		{
			this->GetApp()->GetTaskComponent()->Sleep(3000);
			LOG_ERROR("connect [" << address << "] failure count = " << count++);
		}
		clientContext->SendToServer(request);
		return XCode::Successful;
	}
}

namespace Sentry
{
	XCode RemoteServiceComponent::Call(const std::string& func, long long userId)
	{
		std::string address;
		if(!this->GetEntityAddress(userId, address))
		{
			return XCode::NotFindUser;
		}
		std::shared_ptr<com::Rpc::Request> rpcRequest = this->NewRpcRequest(func, userId, nullptr);
		if(rpcRequest == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}
		std::shared_ptr<com::Rpc::Response> rpcResponse = this->StartCall(address, rpcRequest);
		return rpcResponse != nullptr ? (XCode)rpcResponse->code() : XCode::NetWorkError;
	}

	XCode RemoteServiceComponent::Call(const std::string& func, long long userId, const Message& message)
	{
		std::string address;
		if(!this->GetEntityAddress(userId, address))
		{
			return XCode::NotFindUser;
		}
		std::shared_ptr<com::Rpc::Request> rpcRequest = this->NewRpcRequest(func, userId, &message);
		if(rpcRequest == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}
		std::shared_ptr<com::Rpc::Response> rpcResponse = this->StartCall(address, rpcRequest);
		return rpcResponse != nullptr ? (XCode)rpcResponse->code() : XCode::NetWorkError;
	}

	XCode RemoteServiceComponent::Call(const std::string& func, long long userId, std::shared_ptr<Message> response)
	{
		std::string address;
		assert(response != nullptr);
		if(!this->GetEntityAddress(userId, address))
		{
			return XCode::NotFindUser;
		}
		std::shared_ptr<com::Rpc::Request> rpcRequest = this->NewRpcRequest(func, userId, nullptr);
		if(rpcRequest == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}
		std::shared_ptr<com::Rpc::Response> rpcResponse = this->StartCall(address, rpcRequest);
		if(rpcResponse == nullptr)
		{
			return XCode::NetWorkError;
		}
		if(rpcResponse->code() == (int)XCode::Successful && rpcResponse->has_data())
		{
			rpcResponse->mutable_data()->UnpackTo(response.get());
		}
		return (XCode)rpcResponse->code();
	}

	XCode RemoteServiceComponent::Call(const std::string& func, long long userId, const Message& message, std::shared_ptr<Message> response)
	{
		std::string address;
		assert(response != nullptr);
		if(!this->GetEntityAddress(userId, address))
		{
			return XCode::NotFindUser;
		}
		std::shared_ptr<com::Rpc::Request> rpcRequest = this->NewRpcRequest(func, userId, &message);
		if(rpcRequest == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}
		std::shared_ptr<com::Rpc::Response> rpcResponse = this->StartCall(address, rpcRequest);
		if(rpcResponse == nullptr)
		{
			return XCode::NetWorkError;
		}
		if(rpcResponse->code() == (int)XCode::Successful && rpcResponse->has_data())
		{
			rpcResponse->mutable_data()->UnpackTo(response.get());
		}
		return (XCode)rpcResponse->code();
	}

}
