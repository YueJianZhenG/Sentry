//
// Created by zmhy0073 on 2022/6/6.
//

#include"LuaHttpServiceMethod.h"
#include"Lua/Function.h"
#include"Json/Lua/Json.h"

#include"Lua/LuaServiceTaskSource.h"
#include"Component/ProtoComponent.h"
#include"Component/LuaScriptComponent.h"

namespace Sentry
{
    LuaHttpServiceMethod::LuaHttpServiceMethod(const HttpMethodConfig *config)
        : HttpServiceMethod(config->Method)
    {      
        this->mConfig = config;
        this->mLuaComponent = App::Inst()->GetComponent<LuaScriptComponent>();
    }

    XCode LuaHttpServiceMethod::Invoke(const Http::Request &request, Http::Response &response)
    {
        lua_State* lua = this->mLuaComponent->GetLuaEnv();
        if(this->mConfig->IsAsync && !Lua::Function::Get(lua, "coroutine", "http"))
        {
            return XCode::CallLuaFunctionFail;
        }
        const std::string& service = this->mConfig->Service;
        Lua::LuaModule * luaModule = this->mLuaComponent->GetModule(service);
        if (luaModule == nullptr || !luaModule->GetFunction(this->mConfig->Method))
        {
            Json::Document document;
            document.Add("error", "call lua function not existe");
            document.Add("code", (int)XCode::CallFunctionNotExist);
            response.Json(HttpStatus::OK, document);
            return XCode::CallFunctionNotExist;          
        }
        if (!request.WriteLua(lua))
        {
            lua_pushnil(lua);
        }
        return this->mConfig->IsAsync ? this->CallAsync(response) : this->Call(response);
    }

    XCode LuaHttpServiceMethod::Call(Http::Response &response)
    {
        lua_State* lua = this->mLuaComponent->GetLuaEnv();
        if (lua_pcall(lua, 1, 2, 0) != LUA_OK)
        {
			Json::Document document;
			document.Add("error", lua_tostring(lua, -1));
			document.Add("code", (int)XCode::CallLuaFunctionFail);
			response.Json(HttpStatus::OK, document);
			return XCode::CallLuaFunctionFail;
        }
        if (lua_isstring(lua, -1))
        {
            size_t size = 0;
            const char *json = lua_tolstring(lua, -1, &size);
			response.Json(HttpStatus::OK, json, size);
            return XCode::Successful;
        }
        else if (lua_istable(lua, -1))
        {          
            std::string data;
            Lua::Json::Read(lua, -1, &data);
			response.Json(HttpStatus::OK, data.c_str(), data.size());
            return XCode::Successful;
        }
        return (XCode) lua_tointeger(lua, -2);
    }

    XCode LuaHttpServiceMethod::CallAsync(Http::Response &response)
    {
        lua_State* lua = this->mLuaComponent->GetLuaEnv();       
		std::unique_ptr<LuaServiceTaskSource> luaTaskSource = std::make_unique<LuaServiceTaskSource>(&response);
        Lua::UserDataParameter::Write(lua, luaTaskSource.get());
        if (lua_pcall(lua, 3, 1, 0) != LUA_OK)
        {           
			Json::Document document;
			document.Add("error", lua_tostring(lua, -1));
			document.Add("code", (int)XCode::CallLuaFunctionFail);
			response.Json(HttpStatus::OK, document);
            return XCode::CallLuaFunctionFail;
        }
        XCode code = luaTaskSource->Await();
        return code;
    }
}