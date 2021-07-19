#pragma once
#include<XCode/XCode.h>
#include<Define/CommonDef.h>
#include<Script/LuaParameter.h>
namespace Sentry
{
	class NetLuaRetAction
	{
	public:
		NetLuaRetAction(lua_State * lua, int r) :ref(r), luaEnv(lua) { }
		virtual ~NetLuaRetAction() { luaL_unref(luaEnv, LUA_REGISTRYINDEX, this->ref); }
	public:
		static NetLuaRetAction * Create(lua_State * lua, int index);
	public:
		void Inovke(XCode code);
		void Inovke(XCode code, const Message * message);
		void Inovke(XCode code, const std::string & message);
	private:
		int ref;
		lua_State * luaEnv;
	};
}

namespace Sentry
{
	class NetLuaWaitAction
	{
	public:
		NetLuaWaitAction(lua_State * lua, int r) :ref(r), luaEnv(lua) { }
		virtual ~NetLuaWaitAction() { luaL_unref(luaEnv, LUA_REGISTRYINDEX, this->ref); }
	public:
		static NetLuaWaitAction * Create(lua_State * lua, int index);
	public:
		void Inovke(XCode code);
		void Inovke(XCode code, const Message * message);
		void Inovke(XCode code, const std::string & message);
	private:
		int ref;
		lua_State * luaEnv;
	};
}