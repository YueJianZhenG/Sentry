//
// Created by zmhy0073 on 2022/1/8.
//

#include "LuaServiceTaskSource.h"
#include "Lua/Engine/UserDataParameter.h"
#include "Proto/Lua/Message.h"
#include "XCode/XCode.h"
#include "Entity/Actor/App.h"
#include "Yyjson/Lua/ljson.h"
#include "Server/Config/CodeConfig.h"
namespace acs
{
	LuaServiceTaskSource::LuaServiceTaskSource(http::Response* message)
		: mHttpData(message), mRpcData(nullptr)
    {		
        this->mCode = XCode::LuaCoroutineWait;
    }

	LuaServiceTaskSource::LuaServiceTaskSource(rpc::Message* packet)
		: mHttpData(nullptr), mRpcData(packet)
	{
		this->mCode = XCode::LuaCoroutineWait;
	}

    int LuaServiceTaskSource::Await() noexcept
    {
        if(this->mCode == XCode::LuaCoroutineWait) 
		{
            this->mTaskSource.Await();
        }
        return this->mCode;
    }

	void LuaServiceTaskSource::WriteRpcResponse(lua_State* lua) noexcept
	{
		this->mRpcData->Body()->clear();
		this->mRpcData->SetProto(rpc::proto::none);
		this->mCode = (int)luaL_checkinteger(lua, 2);
		if (this->mCode != XCode::Ok)
		{
			return;
		}
		if (lua_istable(lua, 3))
		{
			const std::string & pb = this->mRpcData->GetAttach();
			if (!pb.empty())
			{
				this->mRpcData->SetProto(rpc::proto::pb);
				pb::Message* message = App::GetProto()->Temp(pb);
				if (message == nullptr)
				{
					this->mCode = XCode::CreateProtoFailure;
					return;
				}
				MessageEncoder messageEncoder(lua);
				if (!messageEncoder.Encode(*message, 3))
				{
					this->mCode = XCode::ParseMessageError;
					return;
				}
				message->SerializePartialToString(this->mRpcData->Body());
			}
			else
			{
				this->mRpcData->SetProto(rpc::proto::json);
				lua::yyjson::read(lua, 3, *this->mRpcData->Body());
			}
		}
		else if (lua_isstring(lua, 3))
		{
			size_t len = 0;
			const char* str = lua_tolstring(lua, 3, &len);
			{
				this->mRpcData->Body()->append(str, len);
				this->mRpcData->SetProto(rpc::proto::string);
			}
		}
	}

	int LuaServiceTaskSource::SetRpc(lua_State* lua) noexcept
	{
		LuaServiceTaskSource* luaServiceTaskSource =
			Lua::UserDataParameter::Read<LuaServiceTaskSource*>(lua, 1);

		luaServiceTaskSource->WriteRpcResponse(lua);
		luaServiceTaskSource->mTaskSource.SetResult();
		return 1;
	}
}

namespace acs
{
	int LuaServiceTaskSource::SetHttp(lua_State* lua) noexcept
	{
		LuaServiceTaskSource* luaServiceTaskSource =
			Lua::UserDataParameter::Read<LuaServiceTaskSource*>(lua, 1);
		if (luaServiceTaskSource->mHttpData != nullptr)
		{
			if(lua_isinteger(lua, 2))
			{
				json::w::Document document;
				int code = (int)luaL_checkinteger(lua, 2);
				document.Add("code", code);
				if(code != XCode::Ok)
				{
					document.Add("error", CodeConfig::Inst()->GetDesc(code));
				}
				if (lua_istable(lua, 3))
				{
					lua::JsonValue jsonValue(true);
					lua::yyjson::read(lua, 3, jsonValue);
					{
						document.Add("data", jsonValue.val);
					}
				}
				else if (lua_isstring(lua, 3))
				{
					size_t size = 0;
					const char* str = lua_tolstring(lua, 3, &size);
					document.Add("data", str, size);
				}
				else if(lua_isinteger(lua, 3))
				{
					document.Add("data", lua_tointeger(lua, 3));
				}
				else if(lua_isnumber(lua, 3))
				{
					document.Add("data", lua_tonumber(lua, 3));
				}
				else if(lua_isboolean(lua, 3))
				{
					document.Add("data", (bool)lua_toboolean(lua, 3));
				}
				luaServiceTaskSource->mCode = code;
				luaServiceTaskSource->mHttpData->SetContent(document);
			}
			else if(lua_isstring(lua, 2))
			{
				size_t size = 0;
				const char* str = lua_tolstring(lua, 3, &size);
				luaServiceTaskSource->mHttpData->Text(str, size);
			}
		}
		luaServiceTaskSource->mTaskSource.SetResult();
		return 1;
	}
}