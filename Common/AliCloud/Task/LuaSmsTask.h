//
// Created by yy on 2024/10/9.
//

#pragma once

#include"Http/Common/HttpResponse.h"
#include"Rpc/Async/RpcTaskSource.h"

namespace acs
{
	class LuaSmsRequestTask final : public IRpcTask<http::Response>
	{
	public:
		explicit LuaSmsRequestTask(int id, lua_State * lua);
		~LuaSmsRequestTask() final;
	public:
		int Await() noexcept;
		void OnTimeout() final;
		void OnResponse(std::unique_ptr<http::Response >& response) noexcept final;
	private:
		int mRef;
		lua_State * mLua;
	};
}
