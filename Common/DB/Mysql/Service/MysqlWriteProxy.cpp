//
// Created by 64658 on 2025/6/6.
//

#include "MysqlWriteProxy.h"
#include "Entity/Actor/App.h"
#include "Proto/Component/ProtoComponent.h"
#include "Mysql/Component/MysqlDBComponent.h"

namespace acs
{
	MysqlWriteProxy::MysqlWriteProxy()
	{
		this->mMysql = nullptr;
	}

	bool MysqlWriteProxy::OnInit()
	{
		BIND_RPC_METHOD(MysqlWriteProxy::Run)
		BIND_RPC_METHOD(MysqlWriteProxy::Inc)
		BIND_RPC_METHOD(MysqlWriteProxy::Update)
		BIND_RPC_METHOD(MysqlWriteProxy::Delete)
		BIND_RPC_METHOD(MysqlWriteProxy::Commit)
		BIND_RPC_METHOD(MysqlWriteProxy::Replace)
		BIND_RPC_METHOD(MysqlWriteProxy::Execute)
		BIND_RPC_METHOD(MysqlWriteProxy::SetIndex)
		BIND_RPC_METHOD(MysqlWriteProxy::InsertOne)
		BIND_RPC_METHOD(MysqlWriteProxy::InsertBatch)
		BIND_RPC_METHOD(MysqlWriteProxy::Deduction)
		LOG_CHECK_RET_FALSE(this->mMysql = this->GetComponent<MysqlDBComponent>())
		return true;
	}

	int MysqlWriteProxy::Run(const std::string & sql, json::w::Document& response)
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

	int MysqlWriteProxy::Inc(const json::r::Document& request, json::w::Document& response)
	{
		int value = 1;
		json::r::Value filter;
		std::string tab, field;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("field", field))
		LOG_ERROR_CHECK_ARGS(request.Get("value", value))
		LOG_ERROR_CHECK_ARGS(request.Get("filter", filter))

		this->mFactory.GetTable(tab).Inc(field, value).Filter(filter);
		const std::string incSql = this->mFactory.Limit(1).ToString();
		this->mFactory.GetTable(tab).Select({ field}).Filter(filter);
		const std::string selectSql = this->mFactory.Limit(1).ToString();
		std::unique_ptr<mysql::Request> mysqlRequest = std::make_unique<mysql::Request>();
		{
			mysqlRequest->AddBatch(incSql);
			mysqlRequest->AddBatch(selectSql);
		}
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(mysqlRequest);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.Add("error", mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		if(!mysqlResponse->contents.empty())
		{
			const std::string & json = mysqlResponse->contents.front();
			json::r::Document document;
			if(document.Decode(json.c_str(), json.size()))
			{
				long long number = 0;
				if(document.Get(field.c_str(), number))
				{
					response.Add("value", number);
				}
			}
		}
		return XCode::Ok;
	}

	int MysqlWriteProxy::InsertOne(const json::r::Document& request, json::w::Document& response)
	{
		std::string tab;
		json::r::Value document;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("document", document))
		this->mFactory.GetTable(tab).Insert(document);
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<mysql::Response> pgsqlResponse = this->mMysql->Run(sql);
		if(pgsqlResponse == nullptr)
		{
			return XCode::Failure;
		}
		if(!pgsqlResponse->IsOk())
		{
			response.Add("error", pgsqlResponse->GetBuffer());
			return XCode::Failure;
		}
		response.Add("count", pgsqlResponse->ok.mAffectedRows);
		return XCode::Ok;
	}

	int MysqlWriteProxy::InsertBatch(const json::r::Document& request, json::w::Document& response)
	{
		std::string tab;
		json::r::Value documents;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("documents", documents) && documents.IsArray())
		std::unique_ptr<mysql::Request> mysqlRequest = std::make_unique<mysql::Request>();
		{
			json::r::Value document;
			for (size_t index = 0; index < documents.MemberCount(); index++)
			{
				LOG_ERROR_CHECK_ARGS(documents.Get(index, document))
				LOG_ERROR_CHECK_ARGS(document.GetType() == YYJSON_TYPE_OBJ)
				mysqlRequest->AddBatch(this->mFactory.GetTable(tab).Insert(document).ToString());
			}
		}
		mysqlRequest->EnableCommit();
		std::unique_ptr<mysql::Response> pgsqlResponse = this->mMysql->Run(mysqlRequest);
		{
			if(pgsqlResponse == nullptr)
			{
				return XCode::Failure;
			}
			if(!pgsqlResponse->IsOk())
			{
				response.Add("error", pgsqlResponse->GetBuffer());
				return XCode::Failure;
			}
			response.Add("count", pgsqlResponse->ok.mAffectedRows);
		}
		return XCode::Ok;
	}

	int MysqlWriteProxy::Replace(const json::r::Document& request, json::w::Document& response)
	{
		std::string tab;
		json::r::Value document;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("document", document))

		this->mFactory.GetTable(tab).Replace(document);
		const std::string sql = this->mFactory.ToString();
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
		return XCode::Ok;
	}

	int MysqlWriteProxy::Delete(const json::r::Document& request, json::w::Document& response)
	{
		std::string tab;
		json::r::Value filter;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("filter", filter))

		int limit = 0;
		this->mFactory.GetTable(tab).Delete().Filter(filter);
		if(request.Get("limit", limit))
		{
			this->mFactory.Limit(limit);
		}
		const std::string sql = this->mFactory.ToString();
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
		return XCode::Ok;
	}

	int MysqlWriteProxy::Update(const json::r::Document& request, json::w::Document& response)
	{
		std::string tab;
		json::r::Value filter;
		json::r::Value document;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("filter", filter))
		LOG_ERROR_CHECK_ARGS(request.Get("document", document) && document.IsObject())

		int limit = 1;
		this->mFactory.GetTable(tab).Update(document).Filter(filter);
		if(request.Get("limit", limit))
		{
			this->mFactory.Limit(limit);
		}
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(sql);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.Add("error", mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		response.Add("count", mysqlResponse->ok.mAffectedRows);
		return XCode::Ok;
	}

	int MysqlWriteProxy::SetIndex(const json::r::Document& request, json::w::Document& response)
	{
		bool unique = false;
		std::string tab, field;
		request.Get("unique", unique);
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("field", field))

		this->mFactory.GetTable(tab).SetIndex(field, unique);
		const std::string sql = this->mFactory.ToString();
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
		return XCode::Ok;
	}

	int MysqlWriteProxy::Execute(const json::r::Document& request, rpc::Message& response)
	{
		std::string stmt;
		json::r::Value jsonArgs;
		LOG_ERROR_CHECK_ARGS(request.Get("stmt", stmt));
		LOG_ERROR_CHECK_ARGS(request.Get("args", jsonArgs) && jsonArgs.IsArray())
		std::string setSql = this->mFactory.Set(jsonArgs).ToString();
		std::string exeSql = this->mFactory.Execute(stmt, jsonArgs).ToString();
		std::unique_ptr<mysql::Request> mysqlRequest = std::make_unique<mysql::Request>();
		{
			mysqlRequest->AddBatch(setSql);
			mysqlRequest->AddBatch(exeSql);
		}
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(mysqlRequest);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.SetError(mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		json::w::Document jsonArray(true);
		for(const std::string & result : mysqlResponse->contents)
		{
			jsonArray.AddObject(result.c_str(), result.size());
		}
		response.SetContent(jsonArray);
		return XCode::Ok;
	}

	int MysqlWriteProxy::Deduction(const json::r::Document& request, json::w::Document& response)
	{
		int value = 0;
		json::r::Value filter;
		std::string tab, field;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("field", field))
		LOG_ERROR_CHECK_ARGS(request.Get("value", value))
		LOG_ERROR_CHECK_ARGS(request.Get("filter", filter))
		if(value <= 0)
		{
			return XCode::CallArgsError;
		}
		std::string update = fmt::format(" UPDATE {} SET {}={}-{}", tab, field, field, value);
		this->mFactory.GetTable(tab).Append(update).Filter(filter).Append(fmt::format(" AND {} >={}", field, value)).Limit(1);
		const std::string sql = this->mFactory.ToString();

		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(sql);
		LOG_ERROR_RETURN_CODE(mysqlResponse != nullptr, XCode::Failure);

		if(!mysqlResponse->IsOk())
		{
			response.Add("error", mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		response.Add("count", mysqlResponse->ok.mAffectedRows);
		return XCode::Ok;
	}



	int MysqlWriteProxy::Commit(const json::r::Document& request, json::w::Document& response)
	{
		size_t count = 0;
		size_t index = 0;
		const char * sql = nullptr;
		std::unique_ptr<mysql::Request> mysqlRequest = std::make_unique<mysql::Request>();
		{
			do
			{
				sql = request.GetString(index++, count);
				if(sql != nullptr && count > 0)
				{
					mysqlRequest->AddBatch(sql, count);
				}
			}
			while(sql != nullptr && count > 0);
		}
		mysqlRequest->EnableCommit();
		LOG_ERROR_CHECK_ARGS(mysqlRequest->Count() > 0);
		std::unique_ptr<mysql::Response> mysqlResponse = this->mMysql->Run(mysqlRequest);
		if(mysqlResponse == nullptr)
		{
			return XCode::Failure;
		}
		if(!mysqlResponse->IsOk())
		{
			response.Add("error", mysqlResponse->GetBuffer());
			return XCode::Failure;
		}
		if(!mysqlResponse->contents.empty())
		{
			auto jsonArray = response.AddArray("list");
			for(const std::string & result : mysqlResponse->contents)
			{
				jsonArray->AddObject(result.c_str(), result.size());
			}
		}
		response.Add("count", mysqlResponse->ok.mAffectedRows);
		return XCode::Ok;
	}
}