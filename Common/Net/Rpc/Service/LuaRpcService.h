#pragma once
#include"RpcService.h"

namespace acs
{
	class LuaRpcService final : public RpcService
	{
	private:
		bool OnInit() final
		{
			if (this->mLuaModule == nullptr)
			{
				LOG_ERROR("not find lua rpc module => {}", this->GetName())
				return false;
			}
			return true;
		}
	};
} // namespace Sentry
