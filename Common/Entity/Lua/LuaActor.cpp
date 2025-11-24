#include "LuaActor.h"
#include "XCode/XCode.h"
#include "Entity/Actor/App.h"
#include "Lua/Engine/Define.h"
#include "Yyjson/Lua/ljson.h"
#include "Cluster/Config/ClusterConfig.h"
#include "Proto/Component/ProtoComponent.h"

namespace acs
{

	int LuaApp::GetPath(lua_State* L)
	{
		size_t count = 0;
		const char *str = luaL_checklstring(L, 1, &count);
		{
			std::string path;
			std::string key(str, count);
			if (!App::Inst()->Config().GetPath(key, path))
			{
				return 0;
			}
			lua_pushlstring(L, path.c_str(), path.size());
		}
		return 1;
	}

	int LuaApp::NewGuid(lua_State* l)
	{
		long long guid = App::Inst()->MakeGuid();
		lua_pushinteger(l, guid);
		return 1;
	}

	int LuaApp::NewUuid(lua_State* l)
	{
		std::string guid = App::Inst()->NewUuid();
		lua_pushlstring(l, guid.c_str(), guid.size());
		return 1;
	}

	inline int LuaPushCode(lua_State* l, int code)
	{
		lua_pushinteger(l, code);
		return 1;
	}

	int LuaActor::Stop(lua_State* L)
	{
		CoroutineComponent * coroutine = App::Get<CoroutineComponent>();
		coroutine->Start(&App::Stop, App::Inst());
		return 0;
	}

	int LuaActor::Send(lua_State* lua)
	{
		static std::string temp;
		size_t size1 = 0, size2 = 0;
		const char * str1 = luaL_checklstring(lua, 1, &size1);
		const char * str2 = luaL_checklstring(lua, 3, &size2);
		long long nodeId = luaL_checkinteger(lua, 2);

		temp.assign(str1, size1);
		IActorComponent * actorComponent = App::Inst()->GetComponent<IActorComponent>(temp);
		if(actorComponent == nullptr)
		{
			LOG_ERROR("not find Component:{}", temp);
			return 0;
		}
		Actor* targetActor = actorComponent->GetActor(nodeId);
		if (targetActor == nullptr)
		{
			LOG_ERROR("not find actor");
			return LuaPushCode(lua, XCode::NotFoundActor);
		}
		temp.assign(str2, size2);
		std::unique_ptr<rpc::Message> message;
		int code = targetActor->MakeMessage(lua, 4, temp, message);
		if (code != XCode::Ok)
		{
			return LuaPushCode(lua, code);
		}
		return targetActor->LuaSend(lua, message);
	}

	int LuaActor::Call(lua_State* lua)
	{
		static std::string temp;
		size_t count1 = 0, count2 = 0;
		const char * str1 = luaL_checklstring(lua, 1, &count1);
		long long actorId =  luaL_checkinteger(lua, 2);
		const char * str2 = luaL_checklstring(lua, 3, &count2);

		temp.assign(str1, count1);
		IActorComponent * actorComponent = App::Inst()->GetComponent<IActorComponent>(temp);
		if(actorComponent == nullptr)
		{
			LOG_ERROR("not find Component:{}", temp);
			return 0;
		}

		Actor * targetActor = actorComponent->GetActor(actorId);
		if (targetActor == nullptr)
		{
			return LuaPushCode(lua, XCode::NotFindUser);
		}
		temp.assign(str2, count2);
		std::unique_ptr<rpc::Message> message;
		int code = targetActor->MakeMessage(lua, 4, temp, message);
		if (code != XCode::Ok)
		{
			return LuaPushCode(lua, code);
		}
		lua_pushthread(lua);
		return targetActor->LuaCall(lua, message);
	}

	int LuaApp::GetConfig(lua_State* lua)
	{
		std::string json = ServerConfig::Inst()->ToString();
		if (!lua::yyjson::write(lua, json.c_str(), json.size()))
		{
			return 0;
		}
		return 1;
	}

	int LuaApp::HasComponent(lua_State* l)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(l, 1, &count);
		{
			std::string name(str, count);
			bool result = App::Inst()->HasComponent(name);
			lua_pushboolean(l, result);
		}
		return 1;
	}

	int LuaActor::GetServers(lua_State* lua)
	{
		size_t count = 0;
		static std::string name;
		const char * str = luaL_checklstring(lua, 1, &count);
		{
			name.assign(str, count);
			NodeComponent* actComponent = App::ActorMgr();
			NodeCluster* nodeCluster = actComponent->GetCluster(name);
			if (nodeCluster == nullptr)
			{
				std::string serverName;
				if (!ClusterConfig::Inst()->GetServerName(name, serverName))
				{
					return 0;
				}
				nodeCluster = actComponent->GetCluster(serverName);
				if (nodeCluster == nullptr)
				{
					return 0;
				}
			}
			const std::vector<int>& items = nodeCluster->GetNodes();
			lua_createtable(lua, 0, (int)items.size());
			for (int index = 0; index < items.size(); index++)
			{
				lua_pushinteger(lua, items[index]);
				lua_seti(lua, -2, index + 1);
			}
		}
		return 1;
	}

	int LuaActor::Broadcast(lua_State* L)
	{
		static std::string temp;
		size_t count1 = 0, count2 = 0;
		const char* str1 = luaL_checklstring(L, 1, &count1);
		const char* str2 = luaL_checklstring(L, 2, &count2);

		temp.assign(str1, count1);
		IActorComponent* actorComponent = App::Inst()->GetComponent<IActorComponent>(temp);
		if (actorComponent == nullptr)
		{
			LOG_ERROR("not find Component:{}", temp);
			return 0;
		}
		temp.assign(str2, count2);
		std::unique_ptr<rpc::Message> message;
		if (acs::App::Inst()->MakeMessage(L, 3, temp, message) != XCode::Ok)
		{
			return 0;
		}
		lua_pushinteger(L, actorComponent->Broadcast(message));
		return 2;
	}
}
