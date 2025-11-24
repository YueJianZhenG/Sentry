//
// Created by 64658 on 2025/6/6.
//

#include "MysqlReadProxy.h"
#include "Util/Tools/String.h"
#include "DB/Common/SqlFactory.h"
#include "Mysql/Component/MysqlDBComponent.h"
namespace acs
{
	MysqlReadProxy::MysqlReadProxy()
	{
		this->mMysql = nullptr;
	}

	bool MysqlReadProxy::OnInit()
	{
		BIND_RPC_METHOD(MysqlReadProxy::Run)
		BIND_RPC_METHOD(MysqlReadProxy::Func)
		BIND_RPC_METHOD(MysqlReadProxy::Find)
		BIND_RPC_METHOD(MysqlReadProxy::Like)
		BIND_RPC_METHOD(MysqlReadProxy::Count)
		BIND_RPC_METHOD(MysqlReadProxy::Execute)
		BIND_RPC_METHOD(MysqlReadProxy::FindOne)
		BIND_RPC_METHOD(MysqlReadProxy::FindPage)
		BIND_RPC_METHOD(MysqlReadProxy::Distinct)
		LOG_CHECK_RET_FALSE(this->mMysql = this->GetComponent<MysqlDBComponent>())
		return true;
	}

	int MysqlReadProxy::Run(const std::string& sql, json::w::Document& response)
	{
		LOG_ERROR_CHECK_ARGS(!sql.empty())
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(sql);
		if(mysqlResponse == nullptr)
		{
			return XCode::Failure;
		}
		if(!mysqlResponse->IsOk())
		{
			response.Add("error", mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		response.Add("count", mysqlResponse->ok.mAffectedRows);
		std::unique_ptr<json::w::Value> jsonArray = response.AddArray("list");
		for(const std::string & result : mysqlResponse->contents)
		{
			jsonArray->AddObject(result.c_str(), result.size());
		}
		return XCode::Ok;
	}

	int MysqlReadProxy::Func(const json::r::Document& request, rpc::Message& response)
	{
		std::string tab, func, field;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab));
		LOG_ERROR_CHECK_ARGS(request.Get("func", func));
		LOG_ERROR_CHECK_ARGS(request.Get("field", field));

		json::r::Value filter;
		this->mFactory.GetTable(tab);
		if(field == "*")
		{
			this->mFactory.Func(func, field);
		}
		else
		{
			this->mFactory.Func(func, field, field);
		}

		std::string group;
		if(request.Get("group", group))
		{
			this->mFactory.Next().Append(group);
		}
		this->mFactory.From();
		if(request.Get("filter", filter))
		{
			this->mFactory.Filter(filter);
		}
		if(!group.empty())
		{
			this->mFactory.GroupBy(group);
		}

		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(sql);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.SetError(mysqlResponse->error.front());
			return XCode::Failure;
		}
		response.ClearHeadAndBody();
		json::w::Document jsonArray(true);
		for(const std::string & json : mysqlResponse->contents)
		{
			jsonArray.PushObject(json);
		}
		response.SetContent(jsonArray);
		return XCode::Ok;
	}

	int MysqlReadProxy::Find(const json::r::Document& request, rpc::Message& response)
	{
		std::string tab;
		json::r::Value fields;
		json::r::Value filter;
		request.Get("fields", fields);
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		this->mFactory.GetTable(tab).Select(fields);

		if(request.Get("filter", filter))
		{
			this->mFactory.Filter(filter);
		}
		if(request.Get("sort", filter))
		{
			this->mFactory.OrderBy(filter);
		}
		int limit = 0;
		if(request.Get("limit", limit))
		{
			this->mFactory.Limit(limit);
		}
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(sql);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.SetError(mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		response.ClearHeadAndBody();
		json::w::Document document(true);
		for(const std::string & result : mysqlResponse->contents)
		{
			document.AddObject(result.c_str(), result.size());
		}
		response.SetContent(document);
		return XCode::Ok;
	}

	int MysqlReadProxy::Like(const json::r::Document& request, rpc::Message& response)
	{
		int limit = 1;
		json::r::Value fields, sort;
		std::string tab, field, value;
		request.Get("limit", limit);
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("field", field))
		LOG_ERROR_CHECK_ARGS(request.Get("value", value))
		LOG_ERROR_CHECK_ARGS(request.Get("fields", fields))
		this->mFactory.GetTable(tab).Select(fields).Like(field, value);
		if(request.Get("sort", sort))
		{
			this->mFactory.OrderBy(sort);
		}
		std::string sql = this->mFactory.Limit(limit).ToString();
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(sql);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.SetError(mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		response.ClearHeadAndBody();
		json::w::Document jsonArray(true);
		for(const std::string & result : mysqlResponse->contents)
		{
			jsonArray.PushObject(result);
		}
		response.SetContent(jsonArray);
		return XCode::Ok;
	}

	int MysqlReadProxy::FindOne(const json::r::Document& request, rpc::Message& response)
	{
		std::string tab;
		json::r::Value filter;
		std::vector<std::string> fields;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("filter", filter))
		{
			this->mFactory.GetTable(tab);
			if(!request.Get("fields", fields))
			{
				this->mFactory.Select();
			}
			else
			{
				this->mFactory.Select(fields);
			}
			this->mFactory.Filter(filter);
		}
		const std::string sql = this->mFactory.Limit(1).ToString();
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(sql);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.SetError(mysqlResponse->error.front());
			return XCode::Failure;
		}
		response.ClearHeadAndBody();
		if(!mysqlResponse->contents.empty())
		{
			const std::string & json = mysqlResponse->contents.front();
			response.SetContent(rpc::proto::json, json.c_str(), json.size());
		}
		return XCode::Ok;
	}

	int MysqlReadProxy::FindPage(const json::r::Document& request, rpc::Message& response)
	{
		int page = 1;
		int count = 1;
		std::string tab;
		std::vector<std::string> fields;
		{
			request.Get("page", page);
			request.Get("count", count);
			request.Get("fields", fields);
		}
		json::r::Value sort;
		json::r::Value filter;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))

		this->mFactory.GetTable(tab).Select(fields);
		if(request.Get("filter", filter))
		{
			this->mFactory.Filter(filter);
		}
		if(request.Get("sort", sort))
		{
			this->mFactory.OrderBy(sort);
		}
		this->mFactory.Page(page, count);
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(sql);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.SetError(mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		response.ClearHeadAndBody();
		json::w::Document document(true);
		for(const std::string & result : mysqlResponse->contents)
		{
			document.AddObject(result.c_str(), result.size());
		}
		response.SetContent(document);
		return XCode::Ok;
	}

	int MysqlReadProxy::Count(const json::r::Document& request, rpc::Message & response)
	{
		std::string tab;
		json::r::Value filter;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab));
		this->mFactory.GetTable(tab).Count();
		if(request.Get("filter", filter))
		{
			this->mFactory.Filter(filter);
		}
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(sql);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.SetError(mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		response.ClearHeadAndBody();
		if(!mysqlResponse->contents.empty())
		{
			response.SetContent(rpc::proto::json, mysqlResponse->contents.front());
		}
		return XCode::Ok;
	}

	int MysqlReadProxy::Execute(const json::r::Document& request, rpc::Message& response)
	{
		std::string stmt;
		json::r::Value jsonArgs;
		LOG_ERROR_CHECK_ARGS(request.Get("stmt", stmt));
		LOG_ERROR_CHECK_ARGS(request.Get("args", jsonArgs))
		std::string setSql = this->mFactory.Set(jsonArgs).ToString();
		std::string exeSql = this->mFactory.Execute(stmt, jsonArgs).ToString();
		std::unique_ptr<mysql::Request> mysqlRequest = std::make_unique<mysql::Request>();
		{
			mysqlRequest->AddBatch(setSql);
			mysqlRequest->AddBatch(exeSql);
		}
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(mysqlRequest);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(mysqlResponse->HasError())
		{
			response.SetError(mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		response.ClearHeadAndBody();
		json::w::Document jsonArray(true);
		for(const std::string & result : mysqlResponse->contents)
		{
			jsonArray.AddObject(result.c_str(), result.size());
		}
		response.SetContent(jsonArray);
		return XCode::Ok;
	}

	int MysqlReadProxy::Distinct(const json::r::Document& request, rpc::Message& response)
	{
		std::string tab;
		json::r::Value filter;
		json::r::Value jsonFields;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("filter", filter))
		LOG_ERROR_CHECK_ARGS(request.Get("fields", jsonFields))
		LOG_ERROR_CHECK_ARGS(filter.IsObject() && jsonFields.IsArray())
		this->mFactory.GetTable(tab).Distinct(jsonFields).Filter(filter);
		std::unique_ptr<mysql::Request> mysqlRequest = std::make_unique<mysql::Request>();
		{
			mysqlRequest->AddBatch(this->mFactory.ToString());
			std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(mysqlRequest);
			LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

			if(!mysqlResponse->IsOk())
			{
				response.SetError(mysqlResponse->GetBuffer());
				return XCode::Failure;
			}
			response.ClearHeadAndBody();
			json::w::Document jsonArray(true);
			for(const std::string & result : mysqlResponse->contents)
			{
				jsonArray.AddObject(result.c_str(), result.size());
			}
			response.SetContent(jsonArray);
		}
		return XCode::Ok;
	}
}