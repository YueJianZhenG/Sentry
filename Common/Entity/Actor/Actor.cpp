//
// Created by yjz on 2023/5/17.
//
#include"App.h"
#include"Actor.h"
#include"XCode/XCode.h"
#include"Rpc/Client/Message.h"
#include"Lua/Engine/LuaParameter.h"
#include"Proto/Include/MessageJson.h"
#include"Util/Json/Lua/Json.h"
#include"Server/Config/ServiceConfig.h"
#include"Rpc/Component/InnerNetComponent.h"
#include"Proto/Component/ProtoComponent.h"
namespace Tendo
{
	Actor::Actor(long long id)
		: Unit(id)
	{
		this->mNetComponent = nullptr;
	}

	bool Actor::LateAwake()
	{
		this->mNetComponent = App::Inst()->GetComponent<InnerNetComponent>();
		LOG_CHECK_RET_FALSE(this->mNetComponent)
		return this->OnInit();
	}

	int Actor::Send(const std::string& func)
	{
		std::string addr;
		if(!this->GetAddress(func, addr))
		{
			return XCode::NotFoundServerRpcAddress;
		}
		const std::shared_ptr<Msg::Packet> message = this->Make(func, nullptr);
		return this->mNetComponent->Send(addr, message) ? XCode::Successful : XCode::SendMessageFail;
	}

	int Actor::Send(const std::string& func, const pb::Message& request)
	{
		std::string addr;
		const int code = this->GetAddress(func, addr);
		if(code != XCode::Successful)
		{
			LOG_ERROR("call " << func << " code =" << code);
			return code;
		}
		const std::shared_ptr<Msg::Packet> message = this->Make(func, &request);
		return this->mNetComponent->Send(addr, message) ? XCode::Successful : XCode::SendMessageFail;
	}

	int Actor::Call(const std::string& func)
	{
		std::string addr;
		const int code = this->GetAddress(func, addr);
		if(code != XCode::Successful)
		{
			LOG_ERROR("call " << func << " code =" << code);
			return code;
		}
		const std::shared_ptr<Msg::Packet> message = std::make_shared<Msg::Packet>();
		{
			message->SetType(Msg::Type::Request);
			message->GetHead().Add("func", func);
		}

		const std::shared_ptr<Msg::Packet> result =
			this->mNetComponent->Call(addr, message);
		return result != nullptr ? result->GetCode() : XCode::NetWorkError;
	}

	std::shared_ptr<Msg::Packet> Actor::Make(const std::string& func, const pb::Message* request) const
	{
		std::shared_ptr<Msg::Packet> message = std::make_shared<Msg::Packet>();
		{
			message->SetType(Msg::Type::Request);
			message->GetHead().Add("func", func);
			message->GetHead().Add("id", this->GetUnitId());
			if(request != nullptr)
			{
				message->SetProto(Msg::Porto::Protobuf);
				message->WriteMessage(request);
			}
		}
		return message;
	}

	int Actor::Call(const std::string& func, const pb::Message& request)
	{
		std::string addr;
		const int code = this->GetAddress(func, addr);
		if(code != XCode::Successful)
		{
			LOG_ERROR("call " << func << " code =" << code);
			return code;
		}
		const std::shared_ptr<Msg::Packet> message = this->Make(func, &request);

		const std::shared_ptr<Msg::Packet> result =
			this->mNetComponent->Call(addr, message);
		return result != nullptr ? result->GetCode() : XCode::NetWorkError;
	}

	int Actor::Call(const std::string& func, std::shared_ptr<pb::Message> response)
	{
		std::string addr;
		int code = this->GetAddress(func, addr);
		if(code != XCode::Successful)
		{
			LOG_ERROR("call " << func << " code =" << code);
			return code;
		}
		const std::shared_ptr<Msg::Packet> message = this->Make(func, nullptr);

		const std::shared_ptr<Msg::Packet> result =
			this->mNetComponent->Call(addr, message);
		code = result != nullptr ? result->GetCode() : XCode::NetWorkError;
		if(code == XCode::Successful)
		{
			if(!result->ParseMessage(response.get()))
			{
				return XCode::ParseMessageError;
			}
		}
		return code;
	}

	int Actor::Call(const std::string& func, const pb::Message& request, std::shared_ptr<pb::Message> response)
	{
		std::string addr;
		int code = this->GetAddress(func, addr);
		if(code != XCode::Successful)
		{
			LOG_ERROR("call " << func << " code =" << code);
			return code;
		}
		const std::shared_ptr<Msg::Packet> message = this->Make(func, &request);

		const std::shared_ptr<Msg::Packet> result =
			this->mNetComponent->Call(addr, message);
		code = result != nullptr ? result->GetCode() : XCode::NetWorkError;
		if(code == XCode::Successful)
		{
			if(!result->ParseMessage(response.get()))
			{
				return XCode::ParseMessageError;
			}
		}
		return code;
	}

	int Actor::LuaSendEx(lua_State* lua)
	{
		const long long actorId = luaL_checkinteger(lua, 1);
		const std::string func(luaL_checkstring(lua, 2));
		Actor * actor = App::Inst()->ActorMgr()->GetActor(actorId);
		if(actor == nullptr)
		{
			lua_pushinteger(lua, XCode::NotFindUser);
			return 1;
		}
		std::shared_ptr<Msg::Packet> message;
		const int code = actor->MakeMessage(lua, 3, func, message);
		if(code != XCode::Successful)
		{
			lua_pushinteger(lua, code);
			return 1;
		}
		return actor->LuaSend(lua, func, message);
	}

	int Actor::LuaCallEx(lua_State* lua)
	{
		const long long actorId = luaL_checkinteger(lua, 1);
		const std::string func(luaL_checkstring(lua, 2));
		Actor * actor = App::Inst()->ActorMgr()->GetActor(actorId);
		if(actor == nullptr)
		{
			lua_pushinteger(lua, XCode::NotFindUser);
			return 1;
		}
		std::shared_ptr<Msg::Packet> message;
		const int code = actor->MakeMessage(lua, 3, func, message);
		if(code != XCode::Successful)
		{
			lua_pushinteger(lua, code);
			return 1;
		}
		return actor->LuaCall(lua, func, message);
	}

	int Actor::MakeMessage(lua_State* lua, int idx,
		const std::string& func, std::shared_ptr<Msg::Packet>& message) const
	{
		App* app = App::Inst();
		const RpcMethodConfig* methodConfig = RpcConfig::Inst()->GetMethodConfig(func);
		if (methodConfig == nullptr)
		{
			LOG_ERROR("not found rpc config" << func);
			return XCode::NotFoundRpcConfig;
		}

		if (lua_istable(lua, idx))
		{
			if (!methodConfig->Request.empty())
			{
				std::shared_ptr<pb::Message> request;
				ProtoComponent* protoComponent = app->GetProto();
				const std::string& name = methodConfig->Request;
				if (!protoComponent->Read(lua, name, idx, request))
				{
					return XCode::CreateProtoFailure;
				}
				message = this->Make(func, request.get());
				return XCode::Successful;
			}
			message = this->Make(func, nullptr);
			message->SetProto(Msg::Porto::Json);
			Lua::RapidJson::Read(lua, idx, message->Body());
			return XCode::Successful;
		}
		if (lua_isstring(lua, idx))
		{
			size_t count = 0;
			message = this->Make(func, nullptr);
			const char* str = lua_tolstring(lua, idx, &count);
			message->Body()->append(str, count);
			message->SetProto(Msg::Porto::String);
			return XCode::Successful;
		}
		return XCode::CallArgsError;
	}

	int Actor::LuaCall(lua_State* lua, const std::string & func, const std::shared_ptr<Msg::Packet> & message)
	{
		std::string address;
		if(!this->GetAddress(func, address))
		{
			lua_pushinteger(lua, XCode::NotFoundPlayerRpcAddress);
			return 1;
		}
		return this->mNetComponent->LuaCall(lua, address, message);
	}

	int Actor::LuaSend(lua_State* lua, const string& func, const std::shared_ptr<Msg::Packet>& message)
	{
		std::string address;
		int code = XCode::Successful;
		do
		{
			if(!this->GetAddress(func, address))
			{
				code = XCode::NotFoundPlayerRpcAddress;
				break;
			}
			if(!this->mNetComponent->Send(address, message))
			{
				code = XCode::SendMessageFail;
				break;
			}
		}
		while(false);
		lua_pushinteger(lua, code);
		return 1;
	}
}