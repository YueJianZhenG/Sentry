//
// Created by 64658 on 2025/1/13.
//

#ifndef APP_LIB_H
#define APP_LIB_H
#include "Event/Base/IEvent.h"
#include "Lua/Engine/ModuleClass.h"

namespace lua
{

	namespace excel
	{
		extern int New(lua_State * L);
	}
	namespace guid
	{
		extern int Gen(lua_State * L);
		extern int Make(lua_State * L);
	}

	namespace lfmt
	{
		extern int lprint(lua_State * L);
		extern int format(lua_State * L);
		extern int serialize(lua_State * L);
		extern int deserialize(lua_State * L);
		extern int deserialize(lua_State * L, const std::string & lua);
		extern int serialize(lua_State* L, int index, std::string& result);
	}

	namespace lib
	{
		extern int luaopen_lfs(lua_State* L);
		extern int luaopen_lfmt(lua_State* L);
		extern int luaopen_lmd5(lua_State* L);
		extern int luaopen_lapp(lua_State* L);
		extern int luaopen_lnode(lua_State* L);
		extern int luaopen_lactor(lua_State* L);
		extern int luaopen_lrouter(lua_State* L);

		extern int luaopen_llog(lua_State* L);
		extern int luaopen_lxml(lua_State* L);
		extern int luaopen_ljson(lua_State* L);
		extern int luaopen_lhttp(lua_State* L);
		extern int luaopen_lbson(lua_State * L);
		extern int luaopen_lguid(lua_State * L);


		extern int luaopen_ltcp(lua_State * L);
		extern int luaopen_lzip(lua_State * L);
		extern int luaopen_loss(lua_State* L);
		extern int luaopen_lsms(lua_State* L);

		extern int luaopen_lmath(lua_State * L);

		extern int luaopen_lcensor(lua_State * L);
#ifdef __ENABLE_OPEN_SSL__
		extern int luaopen_lrsa(lua_State * L);
		extern int luaopen_lraes(lua_State * L);
#endif
		extern int luaopen_ljwt(lua_State* L);
		extern int luaopen_lsha1(lua_State * L);

		extern int luaopen_lproto(lua_State* L);
		extern int luaopen_lbase64(lua_State* L);
		extern int luaopen_ltimer(lua_State* L);

		extern int luaopen_lpgsqldb(lua_State* L);
		extern int luaopen_lmysqldb(lua_State* L);
		extern int luaopen_lredisdb(lua_State* L);
		extern int luaopen_lmonogodb(lua_State* L);
		extern int luaopen_lsqlitedb(lua_State* L);

		extern int luaopen_lsub_redisdb(lua_State* L);

		extern int luaopen_lexcel(lua_State * L);
	}
}

DEFINE_STATIC_EVENT(LuaCCModuleRegister, Lua::CCModule &);



#endif //APP_LIB_H
