﻿#include"MysqlService.h"
#include"DB/Mysql/MysqlTaskSource.h"
#include"Component/Scene/ThreadPoolComponent.h"
#include"App/App.h"
#include"Pool/MessagePool.h"
namespace Sentry
{
	bool MysqlService::Awake()
	{
		BIND_RPC_FUNCTION(MysqlService::Add);
		BIND_RPC_FUNCTION(MysqlService::Save);
		BIND_RPC_FUNCTION(MysqlService::Query);
		BIND_RPC_FUNCTION(MysqlService::Delete);
		BIND_RPC_FUNCTION(MysqlService::Invoke);
		BIND_RPC_FUNCTION(MysqlService::Update);
		return true;
	}

	bool MysqlService::LateAwake()
	{
		RpcServiceNode::LateAwake();
		return this->mHelper.StartConnect();
	}

	XCode MysqlService::Add(const s2s::Mysql::Add& request, s2s::Mysql::Response& response)
	{
		LOGIC_THROW_ERROR(request.has_data());
		LOGIC_THROW_ERROR(!request.table().empty());

		std::string sql;
		if (!this->mHelper.ToSqlCommand(request, sql))
		{
			return XCode::CallArgsError;
		}
		std::shared_ptr<MysqlTaskSource> taskSource(new MysqlTaskSource(this->mHelper));

		XCode code = taskSource->Await(sql);
		if (code != XCode::Successful)
		{
			response.set_error(taskSource->GetErrorStr());
			return code;
		}
		return XCode::Successful;
	}

	XCode MysqlService::Save(const s2s::Mysql::Save& request, s2s::Mysql::Response& response)
	{
		LOGIC_THROW_ERROR(request.has_data());
		LOGIC_THROW_ERROR(!request.table().empty());

		std::string sql;
		if (!this->mHelper.ToSqlCommand(request, sql))
		{
			return XCode::CallArgsError;
		}
		std::shared_ptr<MysqlTaskSource> taskSource =
			std::make_shared<MysqlTaskSource>(this->mHelper);

		XCode code = taskSource->Await(sql);
		if (code != XCode::Successful)
		{
			response.set_error(taskSource->GetErrorStr());
#ifdef __DEBUG__
			LOG_INFO(sql);
			LOG_ERROR(taskSource->GetErrorStr());
#endif
			return code;
		}
		return XCode::Successful;
	}

	XCode MysqlService::Update(const s2s::Mysql::Update& request, s2s::Mysql::Response& response)
	{
		LOGIC_THROW_ERROR(!request.table().empty());
		LOGIC_THROW_ERROR(!request.where_json().empty());

		std::string sql;
		if (!this->mHelper.ToSqlCommand(request, sql))
		{
			return XCode::CallArgsError;
		}
		std::shared_ptr<MysqlTaskSource> taskSource =
			std::make_shared<MysqlTaskSource>(this->mHelper);

		XCode code = taskSource->Await(sql);
		if (code != XCode::Successful)
		{
			response.set_error(taskSource->GetErrorStr());
#ifdef __DEBUG__
			LOG_INFO(sql);
			LOG_ERROR(taskSource->GetErrorStr());
#endif
			return code;
		}
		return XCode::Successful;
	}

	XCode MysqlService::Delete(const s2s::Mysql::Delete& request, s2s::Mysql::Response& response)
	{
		LOGIC_THROW_ERROR(!request.table().empty());
		LOGIC_THROW_ERROR(!request.where_json().empty());

		std::string sql;
		if (!this->mHelper.ToSqlCommand(request, sql))
		{
			return XCode::CallArgsError;
		}
		std::shared_ptr<MysqlTaskSource> taskSource =
			std::make_shared<MysqlTaskSource>(this->mHelper);

		XCode code = taskSource->Await(sql);
		if (code != XCode::Successful)
		{
			response.set_error(taskSource->GetErrorStr());
#ifdef __DEBUG__
			LOG_INFO(sql);
			LOG_ERROR(taskSource->GetErrorStr());
#endif
			return code;
		}
		return XCode::Successful;
	}

	XCode MysqlService::Invoke(const s2s::Mysql::Invoke& request, s2s::Mysql::Response& response)
	{
		LOGIC_THROW_ERROR(!request.sql().empty());
		std::shared_ptr<MysqlTaskSource> taskSource(new MysqlTaskSource(this->mHelper));

		XCode code = taskSource->Await(request.sql());
		if (code != XCode::Successful)
		{
#ifdef __DEBUG__
			LOG_INFO(request.sql());
			LOG_ERROR(taskSource->GetErrorStr());
#endif
			response.set_error(taskSource->GetErrorStr());
			return code;
		}

		while (taskSource->GetQueryData(this->mJson))
		{
			response.add_json_array(this->mJson);
		}
		return XCode::Successful;
	}

	XCode MysqlService::Query(const s2s::Mysql::Query& request, s2s::Mysql::Response& response)
	{
		LOGIC_THROW_ERROR(!request.table().empty());
		LOGIC_THROW_ERROR(!request.where_json().empty());
		std::string sql;
		if (!this->mHelper.ToSqlCommand(request, sql))
		{
			return XCode::CallArgsError;
		}
		std::shared_ptr<MysqlTaskSource> taskSource
			= std::make_shared<MysqlTaskSource>(this->mHelper);

		XCode code = taskSource->Await(sql);
		if (code != XCode::Successful)
		{
			response.set_error(taskSource->GetErrorStr());
			return code;
		}

		while (taskSource->GetQueryData(this->mJson))
		{
			response.add_json_array(this->mJson);
		}

		return XCode::Successful;
	}
}// namespace Sentry