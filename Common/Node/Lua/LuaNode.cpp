//
// Created by 64658 on 2025/4/9.
//

#include "LuaNode.h"
#include "Entity/Actor/App.h"
#include "Node/Component/NodeComponent.h"
#include "Cluster/Config/ClusterConfig.h"

namespace lua
{
	int node::AddListen(lua_State* L)
	{
		acs::Node* server = acs::App::Inst();
		if (lua_isinteger(L, 1))
		{
			int id = (int)lua_tointeger(L, 1);
			server = acs::App::ActorMgr()->Get(id);
			if (server == nullptr)
			{
				luaL_error(L, "not find server:%d", id);
				return 0;
			}
		}
		size_t count1, count2 = 0;
		const char * str1 = luaL_checklstring(L, 2, &count1);
		const char * str2 = luaL_checklstring(L, 3, &count2);

		std::string name(str1, count1);
		std::string address(str2, count2);
		lua_pushboolean(L, server->AddListen(name, address));
		return 1;
	}

	int node::GetListen(lua_State* L)
	{
		std::string address;
		acs::Node* server = acs::App::Inst();
		if (lua_isinteger(L, 1))
		{
			int id = (int)lua_tointeger(L, 1);
			server = acs::App::ActorMgr()->Get(id);
		}
		if (server == nullptr)
		{
			return 0;
		}
		size_t count1 = 0;
		const char * str1 = luaL_checklstring(L, 2, &count1);

		std::string name(str1, count1);
		if (!server->GetListen(name, address))
		{
			return 0;
		}
		lua_pushlstring(L, address.c_str(), address.size());
		return 1;
	}

	int node::AllInfo(lua_State* L)
	{
		acs::Node* server = acs::App::Inst();
		if (lua_isinteger(L, 1))
		{
			int id = (int)lua_tointeger(L, 1);
			server = acs::App::ActorMgr()->Get(id);
		}
		if (server == nullptr)
		{
			return 0;
		}
		lua_newtable(L);
		const std::string& name = server->Name();
		lua_pushinteger(L, server->GetNodeId());
		lua_setfield(L, -2, "id");

		lua_pushlstring(L, name.c_str(), name.size());
		lua_setfield(L, -2, "name");

		lua_newtable(L);
		for(const std::pair<std::string, std::string> & item : server->GetListens())
		{
			lua_pushlstring(L, item.second.c_str(), item.second.size());
			lua_setfield(L, -2, item.first.c_str());
		}
		lua_setfield(L, -2, "listen");
		return 1;
	}

	int node::Remove(lua_State* L)
	{
		int id = (int)luaL_checkinteger(L, 1);
		lua_pushboolean(L, acs::App::ActorMgr()->Remove(id));
		return 1;
	}

	int node::Create(lua_State* L)
	{
		size_t size = 0;
		int id = (int)luaL_checkinteger(L, 1);
		const char * str = luaL_checklstring(L, 2, &size);
		if(str == nullptr || size <= 0)
		{
			return 0;
		}
		std::string name(str, size);
		acs::Node* node = acs::App::ActorMgr()->Get(id);
		if (node == nullptr)
		{
			acs::App::ActorMgr()->Add(std::make_unique<acs::Node>(id, name));
		}
		lua_pushboolean(L, true);
		return 1;
	}

	int node::Query(lua_State* L)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 1, &count);
		if(str == nullptr || count <= 0)
		{
			return 0;
		}
		std::string nodeName;
		std::string service = {str, count};
		if (!acs::ClusterConfig::Inst()->GetServerName(service, nodeName))
		{
			return 0;
		}
		lua_pushlstring(L, nodeName.c_str(), nodeName.size());
		return 1;
	}

	int node::Next(lua_State* L)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 1, &count);
		if(str == nullptr || count <= 0)
		{
			return 0;
		}
		std::string nodeName;
		std::string service(str, count);
		if (!acs::ClusterConfig::Inst()->GetServerName(service, nodeName))
		{
			LOG_ERROR("[{}] find node name", service);
			return 0;
		}
		acs::Node * node = acs::App::ActorMgr()->Next(nodeName);
		if(node == nullptr)
		{
			return 0;
		}
		lua_pushinteger(L, node->GetNodeId());
		return 1;
	}

	int node::Rand(lua_State* L)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 1, &count);
		if(str == nullptr || count <= 0)
		{
			return 0;
		}
		std::string nodeName;
		std::string service(str, count);
		if (!acs::ClusterConfig::Inst()->GetServerName(service, nodeName))
		{
			LOG_ERROR("[{}] find node name", service);
			return 0;
		}
		acs::Node * node = acs::App::ActorMgr()->Rand(nodeName);
		if(node == nullptr)
		{
			return 0;
		}
		lua_pushinteger(L, node->GetNodeId());
		return 1;
	}

	int node::Hash(lua_State* L)
	{
		size_t size = 0;
		std::string nodeName;
		const char * str = luaL_checklstring(L, 1, &size);
		if(str == nullptr || size <= 0)
		{
			return 0;
		}
		std::string service(str, size);
		if (!acs::ClusterConfig::Inst()->GetServerName(service, nodeName))
		{
			LOG_ERROR("[{}] find node name", service);
			return 0;
		}
		long long hash = luaL_checkinteger(L, 2);
		acs::Node * node = acs::App::ActorMgr()->Hash(nodeName, hash);
		if(node == nullptr)
		{
			return 0;
		}
		lua_pushinteger(L, node->GetNodeId());
		return 1;
	}
}