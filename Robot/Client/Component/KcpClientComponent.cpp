//
// Created by 64658 on 2025/9/10.
//

#include "XCode/XCode.h"
#include "Lua/Lib/Lib.h"
#include "Yyjson/Lua/ljson.h"
#include "KcpClientComponent.h"
#include "Util/Tools/String.h"
#include "Util/Tools/TimeHelper.h"
#include "Client/Lua/LuaClient.h"
#include "Rpc/Config/ServiceConfig.h"
#include "Lua/Component/LuaComponent.h"
#include "Proto/Component/ProtoComponent.h"
#include "Rpc/Component/DispatchComponent.h"
namespace acs
{
	KcpClientComponent::KcpClientComponent()
	{
		this->mIndex = 0;
		this->mProto = nullptr;
		this->mLuaComponent = nullptr;
		this->mDisComponent = nullptr;
	}

	bool KcpClientComponent::Awake()
	{
		LuaCCModuleRegister::Add([](Lua::CCModule & moduleClass) {
			moduleClass.Open("net.client", lua::lib::luaopen_lclient);
		});
		return true;
	}

	bool KcpClientComponent::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mProto = this->GetComponent<ProtoComponent>())
		LOG_CHECK_RET_FALSE(this->mLuaComponent = this->GetComponent<LuaComponent>())
		LOG_CHECK_RET_FALSE(this->mDisComponent = this->GetComponent<DispatchComponent>())
		return true;
	}

	void KcpClientComponent::Remove(int id)
	{
		auto iter = this->mClients.find(id);
		if(iter != this->mClients.end())
		{
			this->mClients.erase(iter);
		}
	}

	void KcpClientComponent::OnMessage(rpc::Message* req, rpc::Message* response) noexcept
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

	void KcpClientComponent::OnSystemUpdate(long long now) noexcept
	{
		auto iter = this->mClients.begin();
		for(; iter != this->mClients.end(); iter++)
		{
			iter->second->Update(now);
		}
	}

	int KcpClientComponent::Connect(const std::string& address)
	{
		std::string ip;
		unsigned short port = 0;
		if(!help::Str::SplitAddr(address, ip, port))
		{
			return -1;
		}
		int id = ++this->mIndex;
		Asio::Context & context = this->mApp->GetContext();
		asio_udp::endpoint remote(asio::ip::make_address(ip), port);
		std::shared_ptr<kcp::Client> kcpClient = std::make_shared<kcp::Client>(context, this, remote);
		{
			kcpClient->StartReceive();
			this->mClients.emplace(id, kcpClient);
		}
		return id;
	}

	int KcpClientComponent::Send(int id, std::unique_ptr<rpc::Message>& message) noexcept
	{
		auto iter = this->mClients.find(id);
		if(iter == this->mClients.end())
		{
			return XCode::NetWorkError;
		}
		iter->second->Send(message);
		return XCode::Ok;
	}

	int KcpClientComponent::OnRequest(std::unique_ptr<rpc::Message>& message)
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