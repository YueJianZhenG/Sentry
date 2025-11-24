#include "RpcService.h"
#include "Cluster/Config/ClusterConfig.h"
#include "Lua/Component/LuaComponent.h"
#include "Rpc/Lua/LuaServiceTaskSource.h"
#include "Yyjson/Lua/ljson.h"
#include "Proto/Lua/Message.h"
#include "Lua/Lib/Lib.h"
#include "Core/System/System.h"
#include "Util/Tools/Math.h"
#include "Rpc/Config/ServiceConfig.h"
namespace acs
{
	extern std::string GET_FUNC_NAME(const std::string& fullName)
	{
		size_t pos = fullName.find("::");
		return fullName.substr(pos + 2);
	}

	inline void lua_push_table_field(const rpc::Head & head, const char * key, lua_State* lua, const char* name)
	{
		long long value = 0;
		if(head.Get(key, value))
		{
			lua_pushinteger(lua, value);
			lua_setfield(lua, -2, name);
		}
	}

	RpcService::RpcService()
			: mServiceRegister(this)
	{
		this->mProto = nullptr;
		this->mLuaModule = nullptr;
	}


	bool RpcService::LateAwake()
	{
		const std::string& name = this->GetName();
		LOG_CHECK_RET_FALSE(RpcConfig::Inst()->HasService(name))
		if (!ClusterConfig::Inst()->GetServerName(name, this->mCluster))
		{
			LOG_WARN("{} not find cluster config", name);
		}
		auto* luaComponent = this->GetComponent<LuaComponent>();
		if (luaComponent != nullptr)
		{
			this->mLuaModule = luaComponent->LoadModule(name);
			if (this->mLuaModule != nullptr)
			{
				this->mLuaModule->SetMember("__server", this->mCluster);
			}
		}
		this->mProto = App::GetProto();
		LOG_CHECK_RET_FALSE(this->OnInit());
		return true;
	}

	int RpcService::Invoke(const RpcMethodConfig* methodConfig, std::unique_ptr<rpc::Message> & message)
	{
		const std::string& method = methodConfig->method;
		if (this->mLuaModule != nullptr && this->mLuaModule->HasFunction(method))
		{
			if(!methodConfig->response.empty())
			{
				message->SetAttach(methodConfig->response);
			}
			if (!methodConfig->async)
			{
				return this->CallLua(methodConfig, *message);
			}
			return this->AwaitCallLua(methodConfig, *message);
		}
		ServiceMethod* target = this->mServiceRegister.GetMethod(method);
		return target == nullptr ? XCode::CallFunctionNotExist : target->Invoke(*message);
	}

	int RpcService::WriterToLua(const RpcMethodConfig* config, rpc::Message& message)
	{
		const rpc::Head& head = message.ConstHead();
		lua_State* lua = this->mLuaModule->GetLuaEnv();
		lua_pushlstring(lua, config->method.c_str(), config->method.size());
		lua_createtable(lua, 0, 4);
		{
			tcp::IHeader::WriteLua(lua, head);
			{
				lua_setfield(lua, -2, "head");
			}

			lua_pushinteger(lua, message.SockId());
			lua_setfield(lua, -2, "socketId");

			lua_push_table_field(head, rpc::Header::id.c_str(), lua, "actorId");
			lua_push_table_field(head, rpc::Header::app_id.c_str(), lua, "appId");
			
			switch (message.GetProto())
			{
				case rpc::proto::none:
					return XCode::Ok;
				case rpc::proto::json:
				{
					const std::string& data = message.GetBody();
					if (!data.empty() && !lua::yyjson::write(lua, data.c_str(), data.size()))
					{
						return XCode::ParseJsonFailure;
					}
					break;
				}
				case rpc::proto::lua:
				{
					const std::string& str = message.GetBody();
					if(lua::lfmt::deserialize(lua, str) != LUA_OK)
					{
						return XCode::Failure;
					}
					break;
				}
				case rpc::proto::string:
				{
					const std::string& str = message.GetBody();
					lua_pushlstring(lua, str.c_str(), str.size());
					break;
				}
				case rpc::proto::pb:
				{
					if (!config->request.empty())
					{
						pb::Message* request = this->mProto->Temp(config->request);
						{
							if (request == nullptr)
							{
								return XCode::CreateProtoFailure;
							}
							if (!request->ParsePartialFromString(message.GetBody()))
							{
								return XCode::ParseMessageError;
							}
							this->mProto->Write(lua, *request);
						}
						request->Clear();
					}
					break;
				}
				case rpc::proto::number:
				{
					long long number = 0;
					const std::string& str = message.GetBody();
					if(help::Math::ToNumber(str, number))
					{
						lua_pushinteger(lua, number);
					}
					else
					{
						double value = 0;
						if(help::Math::ToNumber(str, value))
						{
							lua_pushnumber(lua, value);
						}
					}
					break;
				}
				default:
					return XCode::UnKnowPacket;
			}
			lua_setfield(lua, -2, "data");
		}
		return XCode::Ok;
	}

	int RpcService::CallLua(const RpcMethodConfig* config, rpc::Message& message)
	{
		this->mLuaModule->GetMetaFunction("__Invoke");
		lua_State* lua = this->mLuaModule->GetLuaEnv();
		int code = this->WriterToLua(config, message);
		if (code != XCode::Ok)
		{
			return code;
		}
		if (lua_pcall(lua, 3, 2, 0) != LUA_OK)
		{
			std::string err;
			this->mLuaModule->SplitError(err);
			message.GetHead().Add("error", err);
			LOG_ERROR("{} {}", config->fullname, err);
			return XCode::CallLuaFunctionFail;
		}
		message.SetProto(rpc::proto::none);
		if (!lua_isinteger(lua, -2))
		{
			LOG_ERROR("invoke ({}) return code unknown", config->fullname);
			return XCode::Failure;
		}
		code = lua_tointeger(lua, -2);
		switch (lua_type(lua, -1))
		{
			case LUA_TTABLE:
			{
			    pb::Message* data = this->mProto->Temp(config->response);
			    if (data != nullptr)
			    {
			        message.SetProto(rpc::proto::pb);
			        MessageEncoder messageEncoder(lua);
			        if (!messageEncoder.Encode(*data, -1))
			        {
			            return XCode::ParseMessageError;
			        }
			        if(!data->ParsePartialFromString(message.GetBody()))
			        {
			            return XCode::ParseMessageError;
			        }
			    }
			    else
			    {
			        message.SetProto(rpc::proto::json);
			        lua::yyjson::read(lua, -1, *message.Body());
			    }
			    break;
			}
			case LUA_TSTRING:
			{
				size_t len = 0;
				message.Body()->clear();
				const char* str = lua_tolstring(lua, -1, &len);
				message.Body()->append(str, len);
				message.SetProto(rpc::proto::string);
				break;
			}
		}
		return code;
	}

	int RpcService::AwaitCallLua(const RpcMethodConfig* config, rpc::Message& message)
	{
		this->mLuaModule->GetMetaFunction("__Call");
		lua_State* lua = this->mLuaModule->GetLuaEnv();
		if (this->WriterToLua(config, message) != XCode::Ok)
		{
			return XCode::CallLuaFunctionFail;
		}
		std::unique_ptr<LuaServiceTaskSource> luaTaskSource =
				std::make_unique<LuaServiceTaskSource>(&message);

		Lua::UserDataParameter::Write(lua, luaTaskSource.get());
		if (lua_pcall(lua, 4, 1, 0) != LUA_OK)
		{
			std::string err;
			this->mLuaModule->SplitError(err);
			LOG_ERROR("{} {}", config->fullname, err);
			return XCode::CallLuaFunctionFail;
		}
		return luaTaskSource->Await();
	}
}