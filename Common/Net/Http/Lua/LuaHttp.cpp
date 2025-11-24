//
// Created by mac on 2022/5/30.
//

#include"LuaHttp.h"
#include"Yyjson/Lua/ljson.h"
#include"Util/File/DirectoryHelper.h"
#include"Http/Component/HttpComponent.h"
#include"Http/Client/HttpClient.h"
#include"Proto/Component/ProtoComponent.h"

#include"Http/Common/HttpRequest.h"
#include"Http/Task/HttpTask.h"
#include"Entity/Actor/App.h"
#include"Lua/Engine/UserDataParameter.h"
#include"Http/Component/HttpWebComponent.h"

using namespace acs;
namespace lua
{
	int HttpHead::Get(lua_State* L)
	{
		http::Head * head = Lua::UserDataParameter::Read<http::Head*>(L, 1);
		const char * key = luaL_checkstring(L, 2);
		if(head == nullptr || key == nullptr)
		{
			return 0;
		}
		static std::string value;
		if(!head->Get(key, value))
		{
			return 0;
		}
		lua_pushlstring(L, value.c_str(), value.size());
		return 1;
	}

	int HttpClient::Do(lua_State* lua)
	{
		size_t size = 0;
		static std::string url;
		const char * method = luaL_checkstring(lua, 1);
		const char * str = luaL_checklstring(lua, 2, &size);
		static HttpComponent* httpComponent = App::Get<HttpComponent>();

		url.assign(str, size);
		std::unique_ptr<http::Request> request = std::make_unique<http::Request>(method);
		if(!request->SetUrl(url))
		{
			LOG_ERROR("parse get url : [{}] failure", url)
			return 0;
		}
		static std::string key;
		if(lua_istable(lua, 3))
		{
			lua_pushnil(lua);
			while (lua_next(lua, 3))
			{
				switch(lua_type(lua, -2))
				{
					case LUA_TSTRING:
					{
						static std::string value;
						size_t count1 = 0, count2 = 0;
						const char* k = lua_tolstring(lua, -2, &count1);
						const char * v = lua_tolstring(lua, -1, &count2);
						key.assign(k, count1);
						value.assign(v, count2);
						request->Header().Add(key, value);
						break;
					}
					case LUA_TNUMBER:
					{
						size_t count1 = 0;
						const char* k = lua_tolstring(lua, -2, &count1);

						key.assign(k, count1);
						long long val = lua_tointeger(lua, -1);
						request->Header().Add(key, (int)val);
						break;
					}
					default:
						LOG_ERROR("unknown http head type");
						return 0;
				}
				lua_pop(lua, 1);
			}
		}

		if (lua_isstring(lua, 4))
		{
			const char *data = lua_tolstring(lua, 4, &size);
			{
				std::string contentType = http::Header::TEXT;
				request->Header().Del(http::Header::ContentType, contentType);
				{
					request->SetContent(contentType.c_str(), data, size);
					request->Header().Add(http::Header::ContentLength, (int)size);
				}
			}
		}
		else if (lua_istable(lua, 4))
		{
			wrap::string<true> json;
			if(!lua::yyjson::read(lua, 4, json))
			{
				return false;
			}
			request->SetContent(http::Header::JSON, json.c_str(), json.size());
		}
		int taskId = 0;
		lua_pushthread(lua);
		httpComponent->Send(request, taskId);
		return httpComponent->AddTask(new LuaHttpRequestTask(taskId, lua))->Await();
	}

    int HttpClient::Get(lua_State* L)
	{
		size_t size = 0;
		static std::string url;
		const char* str = luaL_checklstring(L, 1, &size);
		static HttpComponent* httpComponent = App::Get<HttpComponent>();

		url.assign(str, size);
		std::unique_ptr<http::Request> request = std::make_unique<http::Request>("GET");
		if (!request->SetUrl(url))
		{
			LOG_ERROR("parse get url : [{}] failure", str);
			return 0;
		}
		int taskId = 0;
		lua_pushthread(L);
		httpComponent->Send(request, taskId);
		return httpComponent->AddTask(new LuaHttpRequestTask(taskId, L))->Await();
	}

	int HttpClient::Post(lua_State* L)
    {
		size_t size = 0;
		static std::string url;
		const char* str = luaL_checklstring(L, 1, &size);
		static HttpComponent* httpComponent = App::Get<HttpComponent>();

		url.assign(str, size);
        std::unique_ptr<http::Request> request = std::make_unique<http::Request>("POST");
        if (!request->SetUrl(url))
        {
            luaL_error(L, "parse post url : [%s] failure", str);
            return 0;
        }

        if (lua_isstring(L, 2))
        {
            const char *data = lua_tolstring(L, 2, &size);
            request->SetContent(http::Header::TEXT, data, size);
        }
        else if (lua_istable(L, 2))
        {
        	wrap::string<true> json;
			if(!lua::yyjson::read(L, 2, json))
			{
				return false;
			}
			request->SetContent(http::Header::JSON, json.c_str(), json.size());
		}
        else
        {
            //luaL_error(lua, "post parameter error");
			LOG_ERROR("[HTTP POST] {} data is nil", url);
            return 0;
        }

		int taskId = 0;
		lua_pushthread(L);
		httpComponent->Send(request, taskId);
        return httpComponent->AddTask(new LuaHttpRequestTask(taskId, L))->Await();
    }

	int HttpClient::Upload(lua_State* lua)
	{
		size_t size, size2 = 0;
		static std::string url;
		const char* str1 = luaL_checklstring(lua, 1, &size);
		const char* path = luaL_checklstring(lua, 2, &size2);
		static HttpComponent* httpComponent = App::Get<HttpComponent>();

		url.assign(str1, size);
		std::unique_ptr<http::Request> request = std::make_unique<http::Request>("POST");
		if (!request->SetUrl(url))
		{
			LOG_ERROR("parse post url : [{}] failure", url);
			return 0;
		}
		std::string type = http::Header::Bin;
		if(lua_isstring(lua, 3))
		{
			type = lua_tostring(lua, 3);
		}
		std::unique_ptr<http::FileContent> fileData = std::make_unique<http::FileContent>();
		{
			if(!fileData->OpenFile(path, type))
			{
				LOG_ERROR("open file error : {}", path);
				return 0;
			}
		}
		int rpcId = 0;
		request->SetContent(std::move(fileData));
		std::unique_ptr<http::Response> response = std::make_unique<http::Response>();
		{
			lua_pushthread(lua);
			//request->Header().SetKeepAlive(false);
			httpComponent->Send(request, response, rpcId);
		}
		return httpComponent->AddTask(new LuaHttpRequestTask(rpcId, lua))->Await();
	}

	int HttpClient::Download(lua_State* lua)
	{
		size_t size, size2 = 0;
		static std::string url;
		const char* str = luaL_checklstring(lua, 1, &size);
		const char* path = luaL_checklstring(lua, 2, &size2);
        static HttpComponent* httpComponent = App::Get<HttpComponent>();

		url.assign(str, size);
        std::unique_ptr<http::Request> request = std::make_unique<http::Request>("GET");
		std::unique_ptr<http::Response> response = std::make_unique<http::Response>();
		if(!response->OpenOrCreateFile(path))
		{
			LOG_ERROR("open or create fail error : {}", path);
			return 0;
		}
		if (!request->SetUrl(url))
        {
			LOG_ERROR("parse post url : [{}] failure", url);
            return 0;
        }

        int taskId = 0;
		lua_pushthread(lua);
		request->Header().Add("Accept", "*/*");
		//request->Header().Add("User-Agent", "Chrome");
		//request->Header().Add("Accept-Encoding", "gzip, deflate, br");
        httpComponent->Send(request, response, taskId);
        return httpComponent->AddTask(new LuaHttpRequestTask(taskId, lua))->Await();
	}
}