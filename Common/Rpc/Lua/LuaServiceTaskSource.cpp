//
// Created by zmhy0073 on 2022/1/8.
//

#include"LuaServiceTaskSource.h"
#include"Lua/Engine/UserDataParameter.h"
#include"Proto/Lua/Message.h"
#include"Util/Json/Lua/Json.h"
#include"XCode/XCode.h"
namespace Tendo
{
	LuaServiceTaskSource::LuaServiceTaskSource(Http::DataResponse* message)
		: mHttpData(message), mRpcData(nullptr)
    {		
        this->mCode = XCode::LuaCoroutineWait;
    }

	LuaServiceTaskSource::LuaServiceTaskSource(Msg::Packet* packet, std::shared_ptr<Message> & message)
		: mHttpData(nullptr), mRpcData(packet), mMessage(message)
	{
		this->mCode = XCode::LuaCoroutineWait;
	}

    int LuaServiceTaskSource::Await()
    {
        if(this->mCode == XCode::LuaCoroutineWait) 
		{
            this->mTaskSource.Await();
        }
        return this->mCode;
    }

	void LuaServiceTaskSource::WriteRpcResponse(lua_State* lua)
	{
		this->mCode = luaL_checkinteger(lua, 2);
		if(this->mCode == XCode::Successful)
		{
			switch(this->mRpcData->GetProto())
			{
				case Msg::Porto::Json:
				case Msg::Porto::String:
				{
					if (lua_istable(lua, 3))
					{
						std::string json;
						Lua::RapidJson::Read(lua, 3, &json);
						this->mRpcData->SetContent(json);
					}
					else if (lua_isstring(lua, 3))
					{
						size_t len = 0;
						const char* str = lua_tolstring(lua, 3, &len);
						this->mRpcData->SetContent({ str, len });
					}
				}
					break;
				case Msg::Porto::Protobuf:
				{
					if (lua_istable(lua, 3) && this->mMessage != nullptr)
					{
						MessageEncoder messageEncoder(lua);
						if (!messageEncoder.Encode(this->mMessage, 3))
						{
							this->mCode = XCode::ParseMessageError;
						}
						this->mRpcData->WriteMessage(this->mMessage.get());
					}
				}
					break;
			}
		}
	}

	int LuaServiceTaskSource::SetRpc(lua_State* lua)
	{
		LuaServiceTaskSource* luaServiceTaskSource =
			Lua::UserDataParameter::Read<LuaServiceTaskSource*>(lua, 1);

		luaServiceTaskSource->WriteRpcResponse(lua);
		luaServiceTaskSource->mTaskSource.SetResult();
		return 1;
	}
}

namespace Tendo
{
	int LuaServiceTaskSource::SetHttp(lua_State* lua)
	{
		LuaServiceTaskSource* luaServiceTaskSource =
			Lua::UserDataParameter::Read<LuaServiceTaskSource*>(lua, 1);
		luaServiceTaskSource->mCode = (int)luaL_checkinteger(lua, 2);
		if (luaServiceTaskSource->mHttpData != nullptr)
		{
			if (lua_istable(lua, 3))
			{
				std::string data;
				Lua::RapidJson::Read(lua, -1, &data);
				luaServiceTaskSource->mHttpData->Json(HttpStatus::OK, data);
			}
			else if (lua_isstring(lua, 3))
			{
				size_t size = 0;
				const char * json = luaL_checklstring(lua, -1, &size);
				luaServiceTaskSource->mHttpData->Json(HttpStatus::OK, json, size);
			}
		}
		luaServiceTaskSource->mTaskSource.SetResult();
		return 1;
	}
}