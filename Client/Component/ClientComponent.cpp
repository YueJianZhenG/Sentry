﻿
#include"ClientComponent.h"
#include"App/App.h"
#include<Util/StringHelper.h>
#include"Network/TcpRpcClientContext.h"
#include"Script/Client.h"
#include"Script/Extension/Message/Message.h"
#include"google/protobuf/util/json_util.h"
#include"Component/Lua/LuaScriptComponent.h"
#include"Network/Listener/TcpServerComponent.h"
namespace Client
{
	ClientTask::ClientTask(int ms)
        : Sentry::IRpcTask<c2s::rpc::response>(ms)
	{
		this->mTaskId = Guid::Create();
	}

	void ClientTask::OnResponse(std::shared_ptr<c2s::rpc::response> response)
	{
		this->mTask.SetResult(response);
	}

    void ClientTask::OnTimeout()
    {
        std::shared_ptr<c2s::rpc::response> response(new c2s::rpc::response());
        response->set_code((int)XCode::CallTimeout);
        this->mTask.SetResult(response);
    }

}

namespace Client
{
	ClientComponent::ClientComponent()
	{
		this->mPort = 0;
		this->mLuaComponent = nullptr;
        this->mTimerComponent = nullptr;
	}

    void ClientComponent::OnRequest(std::shared_ptr<c2s::rpc::call> t1)
    {
        LOG_INFO("call client func = " << t1->func());
	}

    bool ClientComponent::LateAwake()
    {
        this->mTimerComponent = this->GetComponent<TimerComponent>();
		this->mLuaComponent = this->GetComponent<LuaScriptComponent>();
		lua_State * luaState = this->mLuaComponent->GetLuaEnv();

		std::shared_ptr<Lua::Function> luaFunction = Lua::Function::Create(luaState, "Client", "Awake");
		if (luaFunction != nullptr)
		{
			luaFunction->Action();
		}
		return true;
    }

	void ClientComponent::OnAllServiceStart()
	{
		lua_State * luaState = this->mLuaComponent->GetLuaEnv();
		if(!Lua::Function::Get(luaState, "coroutine", "start"))
		{
			return;
		}
		Lua::lua_getfunction(luaState, "Client", "StartLogic");
		if (!lua_isfunction(luaState, -1))
		{
			return;
		}
		if (lua_pcall(luaState, 1, 0, 0) != 0)
		{
			LOG_ERROR(lua_tostring(luaState, -1));
		}
	}

	std::shared_ptr<c2s::rpc::response> ClientComponent::Call(std::shared_ptr<c2s::rpc::request> request)
	{
		std::shared_ptr<ClientTask> clienRpcTask(new ClientTask(0));
		request->set_rpc_id(clienRpcTask->GetRpcId());
		
		this->AddTask(clienRpcTask);
		this->mTcpClient->SendToServer(request);
//		LOG_DEBUG("call [" << request->method_name()
//			<< "] rpc id = " << clienRpcTask->GetRpcId());
		return clienRpcTask->Await();
	}

	void ClientComponent::OnTimeout(long long rpcId)
	{
		this->OnResponse(rpcId, nullptr);
//		auto iter = this->mRpcTasks.find(rpcId);
//		if (iter != this->mRpcTasks.end())
//		{
//			auto rpcTask = iter->second;
//			this->mRpcTasks.erase(iter);
//			rpcTask->SetResult(nullptr);
//		}
	}

	void ClientComponent::OnAddTask(RpcTask rpctask)
	{
		//LOG_WARN(this->GetName() << " add new task " << rpctask->GetRpcId());
	}

	bool ClientComponent::StartConnect(const std::string& ip, unsigned short port)
	{
        TcpServerComponent * tcpComponent = this->GetComponent<TcpServerComponent>();
		std::shared_ptr<SocketProxy> socketProxy = tcpComponent->CreateSocket(ip, port);
		this->mTcpClient = std::make_shared<TcpRpcClientContext>(socketProxy, this);

		return true;
	}

	void ClientComponent::OnLuaRegister(Lua::ClassProxyHelper& luaRegister)
	{
		luaRegister.BeginRegister<ClientComponent>();
		luaRegister.PushExtensionFunction("Call", Lua::ClientEx::Call);
		luaRegister.PushExtensionFunction("StartConnectAsync", Lua::ClientEx::StartConnect);
	}
}
