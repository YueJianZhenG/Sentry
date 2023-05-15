//
// Created by yjz on 2022/5/15.
//

#include"LuaApp.h"
#include"Entity/Unit/App.h"
#include"XCode/XCode.h"
#include"Util/Json/Lua/Json.h"
#include"Entity/Unit/Player.h"
#include"Server/Config/ServiceConfig.h"
#include"Cluster/Config/ClusterConfig.h"
#include"Rpc/Service/RpcService.h"
#include"Rpc/Async/RpcTaskSource.h"
#include"Lua/Engine/LocalClassProxy.h"
#include"Proto/Component/ProtoComponent.h"
#include"Rpc/Component/InnerNetComponent.h"
#include"Rpc/Component/DispatchComponent.h"
#include"Entity/Component/PlayerMgrComponent.h"
using namespace Tendo;
namespace Lua
{
	int LuaApp::GetComponent(lua_State* lua)
	{
		const char* name = luaL_checkstring(lua, -1);
		Component* component = App::Inst()->GetComponentByName(name);
        if(component == nullptr)
        {
            return 0;
        }
		typedef UserDataParameter::UserDataStruct<Component*> ComponentType;
		return ComponentType::WriteObj(lua, component, name);
	}

	int LuaApp::MakeRequest(lua_State * lua, std::shared_ptr<Msg::Packet> & message, std::string & addr, std::string & response)
	{
		const std::string func = luaL_checkstring(lua, 2);
		const RpcMethodConfig* methodConfig = RpcConfig::Inst()->GetMethodConfig(func);
		if (methodConfig == nullptr)
		{
			luaL_error(lua, "not find rpc config : %s", func.c_str());
			return XCode::NotFoundRpcConfig;
		}
		message = std::make_shared<Msg::Packet>();
		{
			message->SetType(Msg::Type::Request);
			message->GetHead().Add("func", func);
		}
		if(lua_isinteger(lua, 1))
		{
			long long userId = lua_tointeger(lua, 1);
			PlayerMgrComponent * playerMgrComponent = App::Inst()->GetComponent<PlayerMgrComponent>();
			if(playerMgrComponent == nullptr)
			{
				luaL_error(lua, "not find PlayerMgrComponent");
				return XCode::Failure;
			}
			Player * player = playerMgrComponent->GetPlayer(userId);
			if(player == nullptr)
			{
				luaL_error(lua, "not find user : %lld", userId);
				return XCode::NotFindUser;
			}
			if(!player->GetAddr(methodConfig->Server, addr))
			{
				return XCode::NotFoundPlayerRpcAddress;
			}
			message->GetHead().Add("id", userId);
		}
		else if(lua_isstring(lua, 1))
		{
			addr = lua_tostring(lua, 1);
		}
		else
		{
			return XCode::CallArgsError;
		}

		if (lua_istable(lua, 3))
		{
			if (!methodConfig->Request.empty())
			{
				const std::string& name = methodConfig->Request;
				std::shared_ptr<Message> request = App::Inst()->GetProto()->Read(lua, name, 3);
				if (request == nullptr)
				{
					luaL_error(lua, "create request message error : %s", name.c_str());
					return XCode::CreateProtoFailure;
				}
				message->SetProto(Msg::Porto::Protobuf);
				message->WriteMessage(request.get());
			}
			else
			{
				message->SetProto(Msg::Porto::Json);
				Lua::RapidJson::Read(lua, 3, message->Body());
			}
		}
		else if(lua_isstring(lua, 3))
		{
			size_t size = 0;
			message->SetProto(Msg::Porto::String);
			const char * str = luaL_tolstring(lua, 3, &size);
			message->Body()->append(str, size);
		}
		response = methodConfig->Response;
		return XCode::Successful;
	}

	int LuaApp::GetAddr(lua_State * lua)
	{
		std::string server, addr;
		const std::string service = luaL_checkstring(lua, 1);
		if(!ClusterConfig::Inst()->GetServerName(service, server))
		{
			luaL_error(lua, "not find service name : %s", service.c_str());
			return 0;
		}
		if(!App::Inst()->GetAddr(server, addr))
		{
			return 0;
		}
		lua_pushlstring(lua, addr.c_str(), addr.size());
		return 1;
	}

	int LuaApp::Send(lua_State* lua)
	{
		std::string response, addr;
		std::shared_ptr<Msg::Packet> message;
		int code = LuaApp::MakeRequest(lua, message, addr, response);
		if(code != XCode::Successful)
		{
			lua_pushinteger(lua, code);
			return 1;
		}
		lua_pushinteger(lua, App::Inst()->Send(addr, message));
		return 1;
	}

	int LuaApp::Call(lua_State* lua)
	{
		lua_pushthread(lua);
		std::string response, addr;
		std::shared_ptr<Msg::Packet> message;
		int code = LuaApp::MakeRequest(lua, message, addr, response);
		if(code != XCode::Successful)
		{
			lua_pushinteger(lua, code);
			return 1;
		}
		int rpcId = 0;
		App * app = App::Inst();
		InnerNetComponent * netComponent = app->GetComponent<InnerNetComponent>();
		DispatchComponent * dispatchComponent = app->GetComponent<DispatchComponent>();
		if (!netComponent->Send(addr, message, rpcId))
		{
			luaL_error(lua, "send request message error");
			return 0;
		}

		std::shared_ptr<LuaRpcTaskSource> luaRpcTaskSource
				= std::make_shared<LuaRpcTaskSource>(lua, rpcId, response);
		return dispatchComponent->AddTask(rpcId, luaRpcTaskSource)->Await();
	}
}