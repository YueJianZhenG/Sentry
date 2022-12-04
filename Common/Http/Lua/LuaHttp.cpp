//
// Created by mac on 2022/5/30.
//

#include"LuaHttp.h"
#include"Json/Lua/Json.h"
#include"File/DirectoryHelper.h"
#include"Lua/UserDataParameter.h"
#include"Component/HttpComponent.h"
#include"Client/HttpRequestClient.h"
#include"Component/ProtoComponent.h"

#include"Http/HttpRequest.h"
#include"Task/HttpTask.h"
using namespace Sentry;
namespace Lua
{
	int HttpClient::Get(lua_State* lua)
    {
        lua_pushthread(lua);        
        HttpComponent* httpComponent = App::Inst()->GetComponent<HttpComponent>();
        if (httpComponent == nullptr)
        {
            luaL_error(lua, "HttpComponent Is Null");
            return 0;
        }
        size_t size = 0;
        const char* str = luaL_checklstring(lua, 1, &size);
        std::shared_ptr<Http::GetRequest> getRequest = std::make_shared<Http::GetRequest>();
        if (!getRequest->SetUrl(std::string(str, size)))
        {
            luaL_error(lua, "parse get url : [%s] failure", str);
            return 0;
        }
#ifdef __DEBUG__
        LOG_DEBUG("[http GET] url = " << std::string(str, size));
#endif
        std::shared_ptr<LuaHttpRequestTask> luaHttpTask(new LuaHttpRequestTask(lua));
        std::shared_ptr<HttpRequestClient> requestClient = httpComponent->CreateClient();

        long long id = requestClient->Do(getRequest);
        return httpComponent->AddTask(id, luaHttpTask)->Await(requestClient);
    }

	int HttpClient::Post(lua_State* lua)
    {
        lua_pushthread(lua);
        HttpComponent *httpComponent = App::Inst()->GetComponent<HttpComponent>();
        if (httpComponent == nullptr)
        {
            luaL_error(lua, "HttpComponent Is Null");
            return 0;
        }
        size_t size = 0;
        const char* str = luaL_checklstring(lua, 1, &size);
        std::shared_ptr<Http::PostRequest> postRequest = std::make_shared<Http::PostRequest>();
        if (!postRequest->SetUrl(std::string(str, size)))
        {
            luaL_error(lua, "parse post url : [%s] failure", str);
            return 0;
        }
        if (lua_isstring(lua, 2))
        {
            const char *data = luaL_checklstring(lua, 2, &size);
            postRequest->Json(data, size);
        }
        else if (lua_istable(lua, 2))
        {
            std::string json;
            Lua::Json::Read(lua, 2, &json);
            postRequest->Json(json.c_str(), json.size());
        }
        else
        {
            luaL_error(lua, "post parameter error");
            return 0;
        }

        std::shared_ptr<LuaHttpRequestTask> luaHttpTask(new LuaHttpRequestTask(lua));
        std::shared_ptr<HttpRequestClient> requestClient = httpComponent->CreateClient();

#ifdef __DEBUG__
        //LOG_DEBUG("[http POST] url = " << std::string(str, size) << " data = " << postRequest->Content());
#endif
        long long id = requestClient->Do(postRequest);
        return httpComponent->AddTask(id, luaHttpTask)->Await(requestClient);
    }

	int HttpClient::Download(lua_State* lua)
	{
        return 0;
	}
}