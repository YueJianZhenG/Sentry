//
// Created by yjz on 2023/3/24.
//



#include "LuaMysql.h"
#include "Entity/Actor/App.h"
#include "Mysql/Lua/LuaMysqlTask.h"
#include "Mysql/Component/MysqlDBComponent.h"
#include "Proto/Component/ProtoComponent.h"
using namespace acs;
namespace lua
{
	int lmysql::Run(lua_State* L)
	{
		size_t size = 0;
		const char * sql = luaL_checklstring(L, 1, &size);
		static MysqlDBComponent* mysql = App::Get<MysqlDBComponent>();

		int rpcId = 0;
		std::unique_ptr<mysql::Request> request = std::make_unique<mysql::Request>(sql, size);
		{
			mysql->Send(request, rpcId);
		}
		lua_pushthread(L);
		return mysql->AddTask(new LuaMysqlTask(L, rpcId))->Await();
	}
}
