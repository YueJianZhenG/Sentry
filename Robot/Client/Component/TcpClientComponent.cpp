//
// Created by leyi on 2023/9/11.
//

#include "TcpClientComponent.h"
#include "Yyjson/Lua/ljson.h"
#include "Util/Tools/String.h"
#include "Lua/Component/LuaComponent.h"
#include "Client/Lua/LuaClient.h"
#include "Lua/Engine/ModuleClass.h"
#include "Rpc/Config/ServiceConfig.h"
#include "Proto/Component/ProtoComponent.h"
#include "Rpc/Component/DispatchComponent.h"
namespace acs
{
	TcpClientComponent::TcpClientComponent()
	{
		this->mIndex = 0;
		this->mProto = nullptr;
		this->mDisComponent = nullptr;
		this->mLuaComponent = nullptr;
	}

	bool TcpClientComponent::Awake()
	{
		LuaCCModuleRegister::Add([](Lua::CCModule & moduleClass) {
			moduleClass.Open("net.client", lua::lib::luaopen_lclient);
		});
		return true;
	}

	bool TcpClientComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mProto = this->GetComponent<ProtoComponent>())
		LOG_CHECK_RET_FALSE(this->mLuaComponent = this->GetComponent<LuaComponent>())
		LOG_CHECK_RET_FALSE(this->mDisComponent = this->GetComponent<DispatchComponent>())
		return true;
	}

	int TcpClientComponent::Connect(const std::string& address)
	{
		std::string ip;
		unsigned short port = 0;
		if(!help::Str::SplitAddr(address, ip, port))
		{
			return -1;
		}
		int id = ++this->mIndex;
		Asio::Context & context = this->mApp->GetContext();
		std::unique_ptr<tcp::Socket> tcpSocket = std::make_unique<tcp::Socket>(context);
		std::shared_ptr<rpc::InnerTcpClient> client = std::make_shared<rpc::InnerTcpClient>(id, this, true, context);
		{
			tcpSocket->Init(ip, port);
			client->SetSocket(tcpSocket.release());
			this->mClientMap.emplace(id, std::move(client));
		}
		return id;
	}

	int TcpClientComponent::Send(int id, std::unique_ptr<rpc::Message>& message) noexcept
	{
		auto iter = this->mClientMap.find(id);
		if(iter == this->mClientMap.end())
		{
			return XCode::NetWorkError;
		}
		return iter->second->Send(message) ? XCode::Ok : XCode::SendMessageFail;
	}

	void TcpClientComponent::OnSendFailure(int id, rpc::Message* message)
	{
		std::unique_ptr<rpc::Message> request(message);
		if(request->GetType() == rpc::type::request && request->GetRpcId() > 0)
		{
			request->SetType(rpc::type::response);
			request->GetHead().Add(rpc::Header::code, XCode::SendMessageFail);
			this->mDisComponent->OnMessage(request);
		}
	}

	void TcpClientComponent::OnMessage(rpc::Message* req, rpc::Message*) noexcept
	{
		std::unique_ptr<rpc::Message> message(req);
		switch(message->GetType())
		{
			case rpc::type::request:
				this->OnRequest(message);
				break;
			case rpc::type::response:
				 this->mDisComponent->OnMessage(message);
				break;
		}
	}

	void TcpClientComponent::Remove(int id)
	{
		auto iter = this->mClientMap.find(id);
		if(iter == this->mClientMap.end())
		{
			return;
		}
		iter->second->Close();
		this->mClientMap.erase(iter);
	}

	void TcpClientComponent::OnClientError(int id, int code)
	{
		auto iter = this->mClientMap.find(id);
		if(iter == this->mClientMap.end())
		{
			return;
		}
		this->mClientMap.erase(iter);
	}

	int TcpClientComponent::OnRequest(std::unique_ptr<rpc::Message> & message)
	{
		const std::string & func = message->GetHead().GetStr(rpc::Header::func);
		const RpcMethodConfig * methodConfig = RpcConfig::Inst()->GetMethodConfig(func);
		if(methodConfig == nullptr)
		{
			return XCode::NotFoundRpcConfig;
		}
		Lua::LuaModule * luaModule = this->mLuaComponent->LoadModule(methodConfig->service);
		if(luaModule == nullptr)
		{
			LOG_ERROR("not find lua client module : {}", methodConfig->service);
			return XCode::CallServiceNotFound;
		}
		lua_State * lua = luaModule->GetLuaEnv();
		luaModule->GetFunction(methodConfig->method);
		const std::string & body = message->GetBody();

		int count = 1;
		switch(message->GetProto())
		{
			case rpc::proto::string:
				count++;
				lua_pushlstring(lua, body.c_str(), body.size());
				break;
			case rpc::proto::json:
				count++;
				lua::yyjson::write(lua, body.c_str(), body.size());
				break;
			case rpc::proto::pb:
			{
				pb::Message * request = this->mProto->Temp(methodConfig->request);
				if(request != nullptr && request->ParsePartialFromString(message->GetBody()))
				{
					count++;
					this->mProto->Write(lua, *request);
				}
				break;
			}
		}
		if(lua_pcall(lua, count, 1, 0) != LUA_OK)
		{
			LOG_ERROR("{}", lua_tostring(lua, -1))
			lua_pop(lua, 1);
		}
		return XCode::Ok;
	}
}