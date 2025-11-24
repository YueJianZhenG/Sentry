//
// Created by mac on 2022/5/31.
//
#include"LuaWaitTaskSource.h"
#include"XCode/XCode.h"
#include "Proto/Lua/Bson.h"
#include"Entity/Actor/App.h"
#include"Async/Lua/LuaCoroutine.h"
#include"Yyjson/Lua/ljson.h"
#include"Proto/Component/ProtoComponent.h"
#include "Util/Tools/Math.h"

namespace acs
{
    LuaWaitTaskSource::LuaWaitTaskSource(lua_State *lua) :
        mRef(0), mLua(lua)
    {

        assert(lua_isthread(this->mLua, -1));
        this->mRef = luaL_ref(lua, LUA_REGISTRYINDEX);
    }

    LuaWaitTaskSource::~LuaWaitTaskSource()
    {
        if (this->mRef != 0)
        {
            luaL_unref(this->mLua, LUA_REGISTRYINDEX, this->mRef);
        }
    }

    int LuaWaitTaskSource::Await()
    {
        if (this->mRef == 0)
        {
            luaL_error(this->mLua, "not lua coroutine context yield failure");
            return 0;
        }
        return lua_yield(this->mLua, 0);
    }

    void LuaWaitTaskSource::SetResult()
    {
        Lua::Coroutine::Resume(this->mLua, 0);
        luaL_unref(this->mLua, LUA_REGISTRYINDEX, this->mRef);
        this->mRef = 0;
    }

    void LuaWaitTaskSource::SetResult(int code, std::unique_ptr<rpc::Message> &response)
    {
        int count = 1;
        lua_pushinteger(this->mLua, code);
        if (code == XCode::Ok && response != nullptr)
        {
            const std::string &message = response->GetBody();
            switch (response->GetProto())
            {
            case rpc::proto::json:
            {
                ++count;
                lua::yyjson::write(this->mLua, message.c_str(), message.size());
                break;
            }
            case rpc::proto::pb:
            {
                const std::string & pb = response->GetAttach();
                pb::Message *message1 = App::GetProto()->Temp(pb);
                if (message1 != nullptr && message1->ParseFromString(message))
                {
                    ++count;
                    App::GetProto()->Write(this->mLua, *message1);
                }
                break;
            }
            case rpc::proto::bson:
            {
                ++count;
                lua::lbson::write(this->mLua, message);
                break;
            }
            case rpc::proto::string:
            {
                ++count;
                lua_pushlstring(this->mLua, message.c_str(), message.size());
                break;
            }
            case rpc::proto::number:
            {
				long long number = 0;
				double floatValue = 0;
                size_t size = message.size();
                const char *str = message.c_str();
                if (help::Math::ToNumber({str, size}, number))
                {
                    ++count;
                    lua_pushinteger(this->mLua, number);
                }
                else if (help::Math::ToNumber({str, size}, floatValue))
                {
                    ++count;
                    lua_pushnumber(this->mLua, floatValue);
                }
                break;
            }
            }
        }
        response->Clear();
        Lua::Coroutine::Resume(this->mLua, count);
        luaL_unref(this->mLua, LUA_REGISTRYINDEX, this->mRef);
        this->mRef = 0;
    }
}
