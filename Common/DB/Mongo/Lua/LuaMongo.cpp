#include"LuaMongo.h"
#include"Entity/Actor/App.h"
#include"Yyjson/Lua/ljson.h"
#include"Mongo/Client/MongoFactory.h"
#include"Mongo/Component/MongoDBComponent.h"


namespace lua
{
	int LuaMongo::Run(lua_State* L)
	{
		size_t count1, count2 = 0;
		using MongoComponent = acs::MongoDBComponent;
		const char* tab = luaL_checklstring(L, 1, &count1);
		const char* cmd = luaL_checklstring(L, 2, &count2);
		static MongoComponent* component = acs::App::Get<MongoComponent>();
		if (component == nullptr)
		{
			luaL_error(L, "not find MongoDBComponent");
			return 0;
		}

		std::unique_ptr<mongo::Request> mongoRequest;
		if (!mongo::MongoFactory::New(tab, cmd, mongoRequest))
		{
			LOG_ERROR("make mongo request error");
			return 0;
		}
		int top = lua_gettop(L);
		for (int index = 3; index <= top - 1; index += 2)
		{
			int valIndex = index + 1;
			if(!lua_isstring(L, index))
			{
				LOG_ERROR("index:{} must string", index);
				return 0;
			}
			const char* key = lua_tostring(L, index);
			switch (lua_type(L, valIndex))
			{
				case LUA_TNUMBER:
				{
					if (lua_isinteger(L, valIndex))
					{
						long long value = lua_tointeger(L, valIndex);
						if (value > std::numeric_limits<int>::max())
						{
							mongoRequest->document.Add(key, value);
						}
						else
						{
							mongoRequest->document.Add(key, (int)value);
						}
					}
					else
					{
						mongoRequest->document.Add(key, lua_tonumber(L, valIndex));
					}
					break;
				}
				case LUA_TSTRING:
				{
					size_t count = 0;
					const char* str = lua_tolstring(L, valIndex, &count);
					mongoRequest->document.Add(key, str, count);
					break;
				}
				case LUA_TBOOLEAN:
				{
					bool value = lua_toboolean(L, valIndex);
					mongoRequest->document.Add(key, value);
					break;
				}
				case LUA_TTABLE:
				{
					wrap::string<true> json;
					if (lua::yyjson::read(L, valIndex, json))
					{
						bson::w::Document document;
						document.FromByJson(json.c_str(), json.size());
						mongoRequest->document.Add(key, document);
					}
					break;
				}
				case LUA_TNIL:
				{
					mongoRequest->document.Add(key);
					break;
				}
				default:
					LOG_ERROR("unknown lua type");
					return 0;
			}
		}
		int taskId = 0;
		lua_pushthread(L);
		component->Send(mongoRequest, taskId);
		return component->AddTask(new acs::LuaMongoTask(L, taskId))->Await();
	}
}
