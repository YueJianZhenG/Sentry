//
// Created by yjz on 2023/5/17.
//

#ifndef APP_ACTORCOMPONENT_H
#define APP_ACTORCOMPONENT_H
#include"Entity/Actor/Server.h"
#include"Entity/Actor/Player.h"
#include"Core/Map/HashMap.h"

namespace Lua
{
	class LuaModule;
};
namespace joke
{
	class ActorComponent : public Component, public IServerRecord, public ILuaRegister
	{
	public:
		Actor * GetActor(long long id);
		bool HasPlayer(long long id) const;
		bool HasServer(long long id) const;
		Player * GetPlayer(long long playerId);
		Server * GetServer(long long serverId);
		bool HasServer(const std::string & name) const;
	public:
		bool AddPlayer(Player * player);
		bool AddServer(Server * server);
		void GetServers(std::vector<int>& servers);
		void GetServers(const std::string &name, std::vector<int>& servers);
		bool GetListen(int id, const std::string & name, std::string & addr);
	public:
		bool UpdateServer(json::r::Document &);
		Server * MakeServer(const json::r::Document &);
	public:
		int Broadcast(const std::string & func);
		int Broadcast(const std::string & func, const pb::Message & message);
	public:
		bool DelActor(long long id);
		bool HasServer(const std::string & name);
		Server * Random(const std::string& name);
		Server * Hash(const std::string & name, long long index);
	private:
		bool LateAwake() final;
		void OnRecord(json::w::Document &document) final;
		void OnLuaRegister(Lua::ModuleClass &luaRegister) final;
		bool AddRandomActor(const std::string& name, Actor* actor);
	private:
		Lua::LuaModule* mLuaModule;
		std::unordered_map<long long, Server *> mServers;
		std::unordered_map<long long, Player *> mPlayers;
		std::unordered_map<std::string, std::vector<long long>> mActorNames;
	};
}

#endif //APP_ACTORCOMPONENT_H
