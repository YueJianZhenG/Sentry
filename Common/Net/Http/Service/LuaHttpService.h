//
// Created by zmhy0073 on 2022/6/6.
//

#ifndef APP_LUAHTTPSERVICE_H
#define APP_LUAHTTPSERVICE_H
#include"HttpService.h"

namespace acs
{
	class LuaHttpService final : public HttpService
	{
	private:
		bool OnInit() final
		{
			if (this->mLuaModule == nullptr)
			{
				LOG_ERROR("not find lua http module => {}", this->GetName())
				return false;
			}
			return true;
		}
	};
}


#endif //APP_LUAHTTPSERVICE_H
