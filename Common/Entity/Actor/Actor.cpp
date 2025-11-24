//
// Created by yjz on 2023/5/17.
//
#include "App.h"
#include "Actor.h"
#include "XCode/XCode.h"
#include "Proto/Lua/Bson.h"
#include "Message/s2s/s2s.pb.h"
#include "Proto/Include/Message.h"
#include "Rpc/Config/ServiceConfig.h"
#include "Yyjson/Lua/ljson.h"
#include "Proto/Component/ProtoComponent.h"
#include "Router/Component/RouterComponent.h"
#include "Lua/Lib/Lib.h"
namespace acs
{
	Actor::Actor(long long id, std::string  name)
		: Entity(id), mName(std::move(name))
	{
		this->mProto = nullptr;
		this->mRouter = nullptr;
	}

	bool Actor::LateAwake()
	{
		LOG_CHECK_RET_FALSE(this->mProto = App::Get<ProtoComponent>())
		LOG_CHECK_RET_FALSE(this->mRouter = App::Get<RouterComponent>())
		return this->OnInit();
	}

	int Actor::Send(const std::string& func) const
	{
		std::unique_ptr<rpc::Message> message = this->Make(func);
		if(message == nullptr)
		{
			return XCode::MakeTcpRequestFailure;
		}
		const int id = message->SockId();
		return this->mRouter->Send(id, message);
	}

	int Actor::Send(const std::string& func, const pb::Message& request) const
	{
		std::unique_ptr<rpc::Message> message = this->Make(func);
		if(message == nullptr)
		{
			return XCode::MakeTcpRequestFailure;
		}
		if(!request.SerializeToString(message->Body()))
		{
			return XCode::SerializationFailure;
		}
		int id = message->SockId();
		message->SetProto(rpc::proto::pb);
		return this->mRouter->Send(id, message);
	}

	int Actor::Send(std::unique_ptr<rpc::Message>& message) const
	{
		int id = message->SockId();
		return this->mRouter->Send(id, message);
	}

	int Actor::Call(const std::string& func) const
	{
		std::unique_ptr<rpc::Message> message = this->Make(func);
		if(message == nullptr)
		{
			return XCode::MakeTcpRequestFailure;
		}
		int id = message->SockId();
		std::unique_ptr<rpc::Message> result = this->mRouter->Call(id, message);
		return result != nullptr ? result->Code() : XCode::NetTimeout;
	}
	
	int Actor::Call(const std::string& func, const pb::Message& request) const
	{
		std::unique_ptr<rpc::Message> message = this->Make(func);
		if(message == nullptr)
		{
			return XCode::MakeTcpRequestFailure;
		}
		if(!request.SerializeToString(message->Body()))
		{
			return XCode::SerializationFailure;
		}
		int id = message->SockId();
		message->SetProto(rpc::proto::pb);
		std::unique_ptr<rpc::Message> result = this->mRouter->Call(id, message);
		return result != nullptr ? result->Code() : XCode::NetWorkError;
	}

	int Actor::Call(const std::string& func, pb::Message * response) const
	{
		std::unique_ptr<rpc::Message> message = this->Make(func);
		if(message == nullptr)
		{
			return XCode::MakeTcpRequestFailure;
		}
		int id = message->SockId();
		std::unique_ptr<rpc::Message> result = this->mRouter->Call(id, message);
		if(result == nullptr)
		{
			return XCode::NetTimeout;
		}
		int code = result->Code();
		const std::string & body = result->GetBody();
		if(code == XCode::Ok && !response->ParseFromString(body))
		{
			return XCode::ParseMessageError;
		}
		return code;
	}

	int Actor::Call(std::unique_ptr<rpc::Message>& message)
	{
		int id = 0;
		if(!this->GetAddress(*message, id))
		{
			return XCode::NotFoundActorAddress;
		}
		std::unique_ptr<rpc::Message> result = this->mRouter->Call(id, message);
		if(result == nullptr)
		{
			return XCode::NetTimeout;
		}
		return result->Code();
	}

	int Actor::Call(const std::string& func, const pb::Message& request, pb::Message * response)
	{
		int code = XCode::Ok;
		do
		{
			std::unique_ptr<rpc::Message> message = this->Make(func);
			if(message == nullptr)
			{
				code = XCode::MakeTcpRequestFailure;
				break;
			}
			if (!request.SerializePartialToString(message->Body()))
			{
				code = XCode::SerializationFailure;
				break;
			}
			int id = message->SockId();
			std::unique_ptr<rpc::Message> result = this->mRouter->Call(id, message);
			if (result == nullptr)
			{
				code = XCode::NetTimeout;
				break;
			}
			code = result->Code();
			const std::string & body = result->GetBody();
			if (code == XCode::Ok && !response->ParsePartialFromString(body))
			{
				code = XCode::ParseMessageError;
				break;
			}
		} while (false);
		return code;
	}

	int Actor::Send(const std::string& func, const json::w::Document& request)
	{
		int code = XCode::Ok;
		do
		{
			std::unique_ptr<rpc::Message> message = this->Make(func);
			if(message == nullptr)
			{
				code = XCode::MakeTcpRequestFailure;
				break;
			}
			if (!request.Serialize(message->Body()))
			{
				code = XCode::SerializationFailure;
				break;
			}
			int id = message->SockId();
			code = this->mRouter->Send(id, message);
		} while (false);
		return code;
	}

	int Actor::Call(const std::string& func, std::unique_ptr<json::r::Document> & response)
	{
		int code = XCode::Ok;
		do
		{
			std::unique_ptr<rpc::Message> message = this->Make(func);
			if(message == nullptr)
			{
				code = XCode::MakeTcpRequestFailure;
				break;
			}
			int id = message->SockId();
			std::unique_ptr<rpc::Message> result = this->mRouter->Call(id, message);
			if (result == nullptr)
			{
				code = XCode::CallTimeout;
				break;
			}
			code = result->Code();
			if(code == XCode::Ok && !response->Decode(result->GetBody()))
			{
				return XCode::ParseJsonFailure;
			}
		}
		while (false);
		return code;
	}

	int Actor::Call(const std::string& func, const json::w::Document& request)
	{
		int code = XCode::Ok;
		do
		{
			std::unique_ptr<rpc::Message> message = this->Make(func);
			if(message == nullptr)
			{
				code = XCode::MakeTcpRequestFailure;
				break;
			}
			if (!request.Serialize(message->Body()))
			{
				code = XCode::SerializationFailure;
				break;
			}
			int id = message->SockId();
			message->SetProto(rpc::proto::json);
			std::unique_ptr<rpc::Message> result = this->mRouter->Call(id, message);
			if (result == nullptr)
			{
				code = XCode::NetTimeout;
				break;
			}
			code = result->Code();
		}
		while (false);
		return code;
	}

	int Actor::Call(const std::string& func, const std::string& request, std::unique_ptr<json::r::Document>& response)
	{
		int code = XCode::Ok;
		do
		{
			std::unique_ptr<rpc::Message> message = this->Make(func);
			if(message == nullptr)
			{
				code = XCode::MakeTcpRequestFailure;
				break;
			}
			int id = message->SockId();
			message->SetContent(rpc::proto::string, request);
			std::unique_ptr<rpc::Message> result = this->mRouter->Call(id, message);
			if (result == nullptr)
			{
				code = XCode::NetTimeout;
				break;
			}
			code = result->Code();
			if(code == XCode::Ok && !response->Decode(result->GetBody()))
			{
				return XCode::ParseJsonFailure;
			}
		} while (false);
		return code;
	}


	int Actor::Call(const std::string& func, const json::w::Document& request, std::unique_ptr<json::r::Document> & response)
	{
		int code = XCode::Ok;
		do
		{
			std::unique_ptr<rpc::Message> message = this->Make(func);
			if(message == nullptr)
			{
				code = XCode::MakeTcpRequestFailure;
				break;
			}
			if (!request.Serialize(message->Body()))
			{
				code = XCode::SerializationFailure;
				break;
			}
			int id = message->SockId();
			message->SetProto(rpc::proto::json);
			std::unique_ptr<rpc::Message> result = this->mRouter->Call(id, message);
			if (result == nullptr)
			{
				code = XCode::NetTimeout;
				break;
			}
			code = result->Code();
			if(code == XCode::Ok && !response->Decode(result->GetBody()))
			{
				return XCode::ParseJsonFailure;
			}
		} while (false);
		return code;
	}


	int Actor::MakeMessage(lua_State* lua, int idx,
		const std::string& func, std::unique_ptr<rpc::Message> & message) const
	{
		const RpcMethodConfig* methodConfig = RpcConfig::Inst()->GetMethodConfig(func);
		if (methodConfig == nullptr)
		{
			LOG_ERROR("call {} fail not rpc config", func);
			return XCode::NotFoundRpcConfig;
		}
		message = this->Make(func);
		if(message == nullptr)
		{
			return XCode::MakeTcpRequestFailure;
		}
		if(methodConfig->proto == rpc::proto::pb
		    && !methodConfig->response.empty())
		{
			message->SetAttach(methodConfig->response);
		}

		switch (lua_type(lua, idx))
		{
			case LUA_TNIL:
				message->SetProto(rpc::proto::none);
				return XCode::Ok;
			case LUA_TSTRING:
			{
				size_t count = 0;
				const char* str = lua_tolstring(lua, idx, &count);
				{
					message->SetContent(str, count);
				}
				return XCode::Ok;
			}
			case LUA_TNUMBER:
			{
				if(lua_isinteger(lua, idx))
				{
					long long number = lua_tointeger(lua, idx);
					message->SetContent(number);
				}
				else
				{
					double number = lua_tonumber(lua, idx);
					message->SetContent(number);
				}
				break;
			}
			case LUA_TTABLE:
			{
				if(methodConfig->proto == rpc::proto::pb)
				{
					if (!methodConfig->request.empty())
					{
						const std::string& name = methodConfig->request;
						pb::Message* request = this->mProto->Read(lua, name, idx);
						if (request == nullptr)
						{
							LOG_ERROR("call {} fail request : {}", func, name);
							return XCode::CreateProtoFailure;
						}
						message->SetProto(rpc::proto::pb);
						if (!request->SerializePartialToString(message->Body()))
						{
							return XCode::SerializationFailure;
						}
						return XCode::Ok;
					}
				}
				else if(methodConfig->proto == rpc::proto::lua)
				{
					if(lua::lfmt::serialize(lua, idx, *message->Body()) != LUA_OK)
					{
						return XCode::Failure;
					}
					message->SetProto(rpc::proto::lua);
					return XCode::Ok;
				}
				else if(methodConfig->proto == rpc::proto::bson)
				{
					if(!lua::lbson::read(lua, idx, *message->Body()))
					{
						LOG_ERROR("read bson fail");
						return XCode::Failure;
					}
					message->SetProto(rpc::proto::bson);
					return XCode::Ok;
				}
				message->SetProto(rpc::proto::json);
				lua::yyjson::read(lua, idx, *message->Body());
				return XCode::Ok;
			}
		}
		return XCode::Ok;
	}

	int Actor::LuaCall(lua_State* lua, std::unique_ptr<rpc::Message>& message) const
	{
		int id = message->SockId();
		return this->mRouter->LuaCall(lua, id, message);
	}

	int Actor::LuaSend(lua_State* lua, std::unique_ptr<rpc::Message>& message) const
	{
		int id = message->SockId();
		int code = this->mRouter->Send(id, message);

		lua_pushinteger(lua, code);
		return 1;
	}
}