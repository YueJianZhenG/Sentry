//
// Created by zmhy0073 on 2021/12/17.
//

#include "Guid.h"

#include "Entity/Actor/App.h"
#include "Util/Tools/TimeHelper.h"

namespace help
{
	long long ID::Gen()
	{
		static int index = 0;
		static long long lastTime = 0;
		long long nowTime = Time::NowTimeMS / 1000;
		if (nowTime != lastTime)
		{
			index = 0;
			lastTime = nowTime;
		}
		return lastTime << 31 | (++index);
	}


	long long ID::Make()
	{
		static int index = 0;
		static long long lastTime = 0;
		long long nowTime = Time::NowTimeMS / 1000;
		static int nodeID = acs::App::Inst()->GetNodeId();
		if (nowTime != lastTime)
		{
			index = 0;
			lastTime = nowTime;
		}
		return lastTime << 31 | nodeID << 16 | (++index);
	}

	long long ID::Make(int count)
	{
		int num = 1;
		for (int index = 0; index < count; index++)
		{
			num *= 10;
		}
		long long guid = ID::Gen();
		return guid % num;
	}
}
