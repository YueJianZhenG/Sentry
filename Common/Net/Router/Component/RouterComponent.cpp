//
// Created by leyi on 2023/6/7.
//

#include "RouterComponent.h"
#include "XCode/XCode.h"
#include "Lua/Lib/Lib.h"
#include "Entity/Actor/App.h"
#include "Rpc/Component/DispatchComponent.h"
#include "Core/Singleton/Singleton.h"

namespace acs
{
	RouterComponent::RouterComponent()
	{
		this->mDispatch = nullptr;
	}

	bool RouterComponent::Awake()
	{
		LuaCCModuleRegister::Add([](Lua::CCModule& ccModule)
		{
			ccModule.Open("core.router", lua::lib::luaopen_lrouter);
		});
		return true;
	}

	bool RouterComponent::LateAwake()
	{
		this->mSenders.fill(nullptr);
		std::vector<rpc::IInnerSender*> senders;
		this->mApp->GetComponents(senders);
		for (rpc::IInnerSender* sender : senders)
		{
			int net = sender->GetNet();
			this->mSenders[net] = sender;
		}
		LOG_CHECK_RET_FALSE(this->mDispatch = this->GetComponent<DispatchComponent>())
		return true;
	}

	int RouterComponent::LuaCall(lua_State* lua, int id, std::unique_ptr<rpc::Message>& message)
	{
		int timeout = message->GetTimeout();
		int rpcId = this->mDispatch->BuildRpcId();
		{
			message->SetRpcId(rpcId);
			int code = this->Send(id, message);
			if (code != XCode::Ok)
			{
				lua_pushinteger(lua, code);
				return 1;
			}
		}
		return this->mDispatch->AddTask(new LuaRpcTaskSource(lua, rpcId), timeout)->Await();
	}

	void RouterComponent::OnSystemUpdate(long long now)
	{
		while (!this->mLocalMessages.empty())
		{
			std::unique_ptr<rpc::Message>& message = this->mLocalMessages.front();
			{
				this->mDispatch->OnMessage(message);
				this->mLocalMessages.pop();
			}
		}
	}

	void RouterComponent::OnRecord(json::w::Document& document)
	{
		auto jsonObject = document.AddObject("router");
		{
			jsonObject->Add("sender", this->mSenders.size());
			jsonObject->Add("local", this->mLocalMessages.size());
		}
	}

	int RouterComponent::Send(int id, std::unique_ptr<rpc::Message>& message)
	{
		int code = XCode::Ok;
		char net = message->GetNet();
		do
		{
			if (message->GetBody().size() >= rpc::INNER_RPC_BODY_MAX_LENGTH)
			{
				code = XCode::NetBigDataShutdown;
				break;
			}

			if (net == rpc::net::client)
			{
				message->SetSource(rpc::source::client);
				if (message->GetBody().size() >= rpc::OUTER_RPC_BODY_MAX_LENGTH)
				{
					code = XCode::NetBigDataShutdown;
					break;
				}
			}
			// else if (this->mApp->Equal(id))
			// {
			// 	message->SetSockId(id);
			// 	this->mLocalMessages.emplace(message.release());
			// 	return XCode::Ok;
			// }
			rpc::IInnerSender* sender = this->GetSender(net);
			if (sender == nullptr)
			{
				LOG_ERROR("not find sender:{}", (int)net);
				code = XCode::NotFoundSender;
				break;
			}
			code = sender->Send(id, message);
		}
		while (false);
		if (code != XCode::Ok)
		{
			int rpcId = message->GetRpcId();
			std::string msg = message->ToString();
			LOG_WARN("({}) [send to {}:{}] ({}) => {}", code, id, (int)net, rpcId, msg);
		}
		return code;
	}

	int RouterComponent::Send(int id, int code, std::unique_ptr<rpc::Message>& message)
	{
		if (code == XCode::CloseSocket)
		{
			message->SetType(rpc::type::close);
		}
		else if (message->GetRpcId() == 0)
		{
			return XCode::Ok;
		}
		message->SetType(rpc::type::response);
		rpc::Head& head = message->GetHead();
		{
			head.Del(rpc::Header::app_id);
			head.Add(rpc::Header::code, code);
		}
		return this->Send(id, message);
	}

	std::unique_ptr<rpc::Message> RouterComponent::Call(int id, std::unique_ptr<rpc::Message>& message)
	{
		int timeout = message->GetTimeout();
		int rpcId = this->mDispatch->BuildRpcId();
		{
			message->SetRpcId(rpcId);
			if (this->Send(id, message) != XCode::Ok)
			{
				return nullptr;
			}
		}
		return this->mDispatch->BuildRpcTask<RpcTaskSource>(rpcId, timeout)->Await();
	}

	void RouterComponent::Broadcast(std::unique_ptr<rpc::Message>& message)
	{
		char net = message->GetNet();
		rpc::IInnerSender* sender = this->GetSender(net);
		if (sender == nullptr)
		{
			LOG_ERROR("not find sender => {}", (int)net);
			return;
		}
		sender->Broadcast(message);
	}
}
