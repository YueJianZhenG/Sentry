
#pragma once

#include "Rpc/Async/RpcTaskSource.h"
#include "Mysql/Common/MysqlProto.h"
struct lua_State;
namespace acs
{

	class LuaMysqlTask : public IRpcTask<mysql::Response>
	{
	public:
		LuaMysqlTask(lua_State* lua, int id);
		~LuaMysqlTask() final;
	public:
		int Await();
		void OnTimeout() final;
		void OnResponse(std::unique_ptr<mysql::Response>& response) noexcept final ;
	private:
		int mRef;
		lua_State* mLua;
	};
}


