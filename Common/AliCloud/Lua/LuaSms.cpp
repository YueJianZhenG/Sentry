//
// Created by yy on 2025/11/21.
//

#include "LuaSms.h"
#include "AliCloud/Component/AliSmsComponent.h"
#include "Entity/Actor/App.h"
#include "Yyjson/Lua/ljson.h"
#include "Http/Common/HttpResponse.h"
#include "Http/Component/HttpComponent.h"
#include "AliCloud/Task/LuaSmsTask.h"
namespace lua
{
	int sms::Send(lua_State * L)
	{
		static std::string phoneNum;
		static std::string paramater;
		static acs::AliSmsComponent * smsComponent = acs::App::Get<acs::AliSmsComponent>();
		static acs::HttpComponent* httpComponent = acs::App::Get<acs::HttpComponent>();
		{
			size_t size2 = 0, size3 = 0;
			const char * str2 =  luaL_checklstring(L, 1, &size2);
			if(lua_istable(L, 2))
			{
				if(!lua::yyjson::read(L, 2, paramater))
				{
					return 0;
				}
			}
			else if(lua_isstring(L, 2))
			{
				json::w::Document document;
				const char * str3 = luaL_tolstring(L, 2, &size3);
				document.Add("code", str3, size3);
				document.Serialize(&paramater);
			}
			else if(lua_isinteger(L, 2))
			{
				json::w::Document document;
				long long num = lua_tointeger(L, 2);
				document.Add("code", std::to_string(num));
				document.Serialize(&paramater);
			}
			else
			{
				return 0;
			}
			phoneNum.assign(str2, size2);
		}
		std::unique_ptr<http::Response> response1 = std::make_unique<http::Response>();
		std::unique_ptr<http::Request> request = smsComponent->MakeRequest(phoneNum, paramater);
		{
			int rpcId = 0;
			lua_pushthread(L);
			httpComponent->Send(request, response1, rpcId);
			return httpComponent->AddTask(new acs::LuaSmsRequestTask(rpcId, L))->Await();
		}
	}
}
