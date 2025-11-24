
#pragma once

#include "Rpc/Async/RpcTaskSource.h"
#include "Pgsql/Common/PgsqlCommon.h"
struct lua_State;
namespace acs
{

	class LuaPgsqlTask : public IRpcTask<pgsql::Response>
	{
	public:
		LuaPgsqlTask(lua_State* lua, int id);
		~LuaPgsqlTask() final;
	public:
		int Await();
		void OnTimeout() final;
		void OnResponse(std::unique_ptr<pgsql::Response>& response) noexcept final ;
	private:
		int mRef;
		lua_State* mLua;
	};
}


