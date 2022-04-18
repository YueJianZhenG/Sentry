﻿#include"RpcComponent.h"
#include<Component/RpcService/LocalServerRpc.h>
#include"Component/Coroutine/TaskComponent.h"
#include"Util/StringHelper.h"
#include"App/App.h"
#include"Pool/MessagePool.h"
#include"Method/LuaServiceMethod.h"
#include"Global/RpcConfig.h"
#include"Component/Rpc/RpcClientComponent.h"
#include"Other/ElapsedTimer.h"
#include"Json/JsonWriter.h"
#include"Async/RpcTask/RpcTaskSource.h"
namespace Sentry
{
	void RpcComponent::Awake()
	{
		this->mCorComponent = nullptr;
		this->mTimerComponent = nullptr;
		this->mRpcClientComponent = nullptr;
	}

	bool RpcComponent::LateAwake()
	{
		this->mCorComponent = App::Get()->GetTaskComponent();
		this->mTimerComponent = this->GetComponent<TimerComponent>();
		LOG_CHECK_RET_FALSE(this->mCorComponent = this->GetComponent<TaskComponent>());
		LOG_CHECK_RET_FALSE(this->mRpcClientComponent = this->GetComponent<RpcClientComponent>());
		return true;
	}

	XCode RpcComponent::OnRequest(std::shared_ptr<com::Rpc_Request> request)
	{
		unsigned short methodId = request->method_id();
		const RpcConfig & rpcConfig = this->GetApp()->GetRpcConfig();
		const ProtoConfig* protocolConfig = rpcConfig.GetProtocolConfig(methodId);
		if (protocolConfig == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}

		const std::string& service = protocolConfig->Service;
		LocalServerRpc * logicService = this->GetComponent<LocalServerRpc>(service);
		if (logicService == nullptr)
		{
			LOG_ERROR("call service not exist : [" << service << "]");
			return XCode::CallServiceNotFound;
		}
#ifdef __DEBUG__
		std::string json = "";
		LOG_DEBUG("==============[request]==============");
		LOG_DEBUG("address = " << request->address());
		LOG_DEBUG("func = " << protocolConfig->Service << "." << protocolConfig->Method);
		if (request->has_data() && Helper::Proto::GetJson(request->data(), json))
		{
			LOG_DEBUG("json = " << json);
		}
		LOG_DEBUG("=====================================");
#endif

		if (!protocolConfig->IsAsync)
		{
			const std::string& method = protocolConfig->Method;
			auto response = logicService->Invoke(method, request);
			this->mRpcClientComponent->Send(request->address(), response);
			return XCode::Successful;
		}
		this->mCorComponent->Start([request, this, logicService, protocolConfig]()
		{
			const std::string& method = protocolConfig->Method;
			std::shared_ptr<com::Rpc::Response> response = logicService->Invoke(method, request);
			this->mRpcClientComponent->Send(request->address(), response);
		});
		return XCode::Successful;
	}

	XCode RpcComponent::OnResponse(std::shared_ptr<com::Rpc_Response> response)
	{
		long long rpcId = response->rpc_id();
		auto iter = this->mRpcTasks.find(rpcId);
		if (iter == this->mRpcTasks.end())
		{
			LOG_WARN("not find rpc task : "<< rpcId)
			return XCode::Failure;
		}
		auto rpcTask = iter->second;
#ifdef __DEBUG__
		int methodId = 0;
		long long time = 0;
		if (this->GetRpcInfo(rpcId, methodId, time))
		{
			std::string json = "";
			const RpcConfig & rpcConfig = this->GetApp()->GetRpcConfig();
			const ProtoConfig* protoConfig = rpcConfig.GetProtocolConfig(methodId);

			LOG_DEBUG("*************[response]*************");
			LOG_DEBUG("func = " << protoConfig->Service << '.' << protoConfig->Method);
			LOG_DEBUG("time = " << time << " ms");
			if (response->has_data() && Helper::Proto::GetJson(response->data(), json))
			{
				LOG_DEBUG("json = " << json);
			}
			LOG_DEBUG("************************************");
		}
#endif
		this->mRpcTasks.erase(iter);
		rpcTask->OnResponse(response);
		return XCode::Successful;
	}

	void RpcComponent::AddRpcTask(std::shared_ptr<IRpcTask> task)
	{
		long long rpcId = task->GetRpcId();
		this->mRpcTasks.emplace(rpcId, task);
		if (task->GetTimeout() > 0)
		{
			this->mTimerComponent->AsyncWait(
				task->GetTimeout(), &RpcComponent::OnTaskTimeout, this, rpcId);
		}
	}
#ifdef __DEBUG__
	void RpcComponent::AddRpcInfo(long long rpcId, int methodId)
	{
		RpcTaskInfo taskInfo;
		taskInfo.MethodId = methodId;
		taskInfo.Time = Helper::Time::GetNowMilTime();
		this->mRpcInfoMap.emplace(rpcId, taskInfo);
	}

	bool RpcComponent::GetRpcInfo(long long int rpcId, int& methodId, long long int& time)
	{
		auto iter = this->mRpcInfoMap.find(rpcId);
		if (iter == this->mRpcInfoMap.end())
		{
			return false;
		}
		methodId = iter->second.MethodId;
		time = Helper::Time::GetNowMilTime() - iter->second.Time;
		this->mRpcInfoMap.erase(iter);
		return true;
	}
#endif

	void RpcComponent::OnTaskTimeout(long long int rpcId)
	{
		auto iter = this->mRpcTasks.find(rpcId);
		if (iter != this->mRpcTasks.end())
		{
			auto rpcTask = iter->second;
			this->mRpcTasks.erase(iter);
			rpcTask->OnResponse(nullptr);
		}
#ifdef __DEBUG__
		int methodId = 0;
		long long costTime = 0;
		if (this->GetRpcInfo(rpcId, methodId, costTime))
		{
			const RpcConfig & rpcConfig = this->GetApp()->GetRpcConfig();
			const ProtoConfig* config = rpcConfig.GetProtocolConfig(methodId);
			if (config != nullptr)
			{
				LOG_ERROR("call " << config->Service << '.' << config->Method << " time out");
			}
		}
#endif
	}
}// namespace Sentry
