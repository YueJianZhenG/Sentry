//
// Created by yjz on 2022/1/23.
//
#include"HttpService.h"
#include"Yyjson/Lua/ljson.h"
#include"Lua/Component/LuaComponent.h"
#include"Rpc/Lua/LuaServiceTaskSource.h"

namespace acs
{

	HttpService::HttpService()
			: mServiceRegister(this)
	{
		this->mLuaModule = nullptr;
	}

	bool HttpService::LateAwake()
	{
		const std::string & name = this->GetName();
		LOG_CHECK_RET_FALSE(HttpConfig::Inst()->HasService(name))
		LuaComponent * luaComponent = this->GetComponent<LuaComponent>();
		if(luaComponent != nullptr)
		{
			this->mLuaModule = luaComponent->LoadModule(name);
		}
		if(!this->OnInit())
		{
			LOG_ERROR("[{}] Init Fail", name)
			return false;
		}
		return true;
	}

	int HttpService::Invoke(const HttpMethodConfig * config, const http::Request& request, http::Response& response)
	{
		const std::string & method = config->method;
		if(this->mLuaModule != nullptr && this->mLuaModule->HasFunction(method))
		{
			if(!config->async)
			{
				return this->CallLua(method, request, response);
			}
			return this->AwaitCallLua(method, request, response);
		}
		HttpServiceMethod * target = this->mServiceRegister.GetMethod(method);
		return target == nullptr ? XCode::CallFunctionNotExist : target->Invoke(request, response);
	}

	int HttpService::CallLua(const std::string & method, const http::Request& request, http::Response& response)
	{
		this->mLuaModule->GetMetaFunction("__Invoke");
		lua_State * lua = this->mLuaModule->GetLuaEnv();
		lua_pushlstring(lua, method.c_str(), method.size());
		{
			if(request.WriteToLua(lua) == -1)
			{
				return XCode::CallArgsError;
			}
			if(lua_pcall(lua, 3, 2, 0) != LUA_OK)
			{
				const char * err = lua_tostring(lua, -1);
				{
					LOG_ERROR("{} error:{}", method, err);
				}
				lua_pop(lua, 1);
				return XCode::CallLuaFunctionFail;
			}
			int code = (int)luaL_checkinteger(lua, -2);
			if(code != XCode::Ok)
			{
				return code;
			}
			if (lua_istable(lua, -1))
			{
				json::w::Document document;
				lua::JsonValue jsonValue(true);
				lua::yyjson::read(lua, -1, jsonValue);
				{
					document.Add("code", code);
					document.Add("data", jsonValue.val);
				}
				response.SetContent(document);
				return XCode::Ok;
			}
			return XCode::Failure;
		}
	}

	int HttpService::AwaitCallLua(const std::string & method, const http::Request& request, http::Response& response)
	{
		if(!this->mLuaModule->GetMetaFunction("__Call"))
		{
			return XCode::CallServiceNotFound;
		}
		lua_State * lua = this->mLuaModule->GetLuaEnv();
		lua_pushlstring(lua, method.c_str(), method.size());
		{
			request.WriteToLua(lua);
			std::unique_ptr<LuaServiceTaskSource> luaTaskSource =
					std::make_unique<LuaServiceTaskSource>(&response);
			Lua::UserDataParameter::Write(lua, luaTaskSource.get());
			if (lua_pcall(lua, 4, 1, 0) != LUA_OK)
			{
				std::string error;
				this->mLuaModule->SplitError(error);
				LOG_ERROR("{} error:{}", method, error);
				return XCode::CallLuaFunctionFail;
			}
			return luaTaskSource->Await();
		}
	}
}