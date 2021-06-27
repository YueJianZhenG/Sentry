﻿#pragma once
#include"MysqlTaskBase.h"
namespace SoEasy
{
	class CoroutineManager;
	class MysqlUpdateTask : public MysqlTaskBase
	{
	public:
		MysqlUpdateTask(MysqlManager * mgr, long long id, const std::string & db);
		~MysqlUpdateTask() { }
	public:
		bool InitTask(const std::string tab, CoroutineManager * corMgr, shared_ptr<Message> data);
	protected:
		void OnTaskFinish() final;
		const std::string & GetSqlCommand() final;
	private:
		std::string mTable;
		long long mCoroutineId;
		std::string mSqlCommand;
		shared_ptr<Message> mData;
		CoroutineManager * mCorManager;
	};
}