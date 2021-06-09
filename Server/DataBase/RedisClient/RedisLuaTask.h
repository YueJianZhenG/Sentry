#pragma once
#include"RedisDefine.h"
#include"RedisTaskBase.h"
#include<Script/LuaInclude.h>

#define RedisLuaArgvSize 10
namespace SoEasy
{
	class QuertJsonWritre;
	class RedisLuaTask : public RedisTaskBase
	{
	public:
		RedisLuaTask(RedisManager * mgr, long long taskId, const std::string & cmd, lua_State * lua, int ref);
		~RedisLuaTask();
	protected:
		void OnTaskFinish() final;  //ִ�����֮�������̵߳���
		void OnQueryFinish(QuertJsonWritre & jsonWriter) final;
	public:
		static shared_ptr<RedisLuaTask> Create(lua_State * lua, int index, const char * cmd);
	private:
		int mCoroutienRef;
		lua_State * mLuaEnv;
	private:
		std::string mQueryJsonData;
	};
}