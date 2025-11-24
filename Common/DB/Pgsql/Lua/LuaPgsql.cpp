//
// Created by yjz on 2023/3/24.
//



#include "LuaPgsql.h"
#include "Entity/Actor/App.h"
#include "Pgsql/Lua/LuaPgsqlTask.h"
#include "Pgsql/Component/PgsqlDBComponent.h"
using namespace acs;
namespace lua
{
	int lpgsql::Run(lua_State* L)
	{
		size_t size = 0;
		const char * sql = luaL_checklstring(L, 1, &size);

		static PgsqlDBComponent* pgsql = App::Get<PgsqlDBComponent>();

		int rpcId = 0;
		std::unique_ptr<pgsql::Request> request = std::make_unique<pgsql::Request>(sql, size);
		{
			pgsql->Send(request, rpcId);
		}
		lua_pushthread(L);
		return pgsql->AddTask(new LuaPgsqlTask(L, rpcId))->Await();
	}
}
