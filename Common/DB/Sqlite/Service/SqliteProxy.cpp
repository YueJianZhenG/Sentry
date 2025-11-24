//
// Created by 64658 on 2025/6/6.
//

#include "SqliteProxy.h"
#include "Entity/Actor/App.h"
#include "Proto/Component/ProtoComponent.h"
#include "Sqlite/Component/SqliteComponent.h"

namespace acs
{
	SqliteProxy::SqliteProxy()
	{
		this->mSqlite = nullptr;
	}

	bool SqliteProxy::OnInit()
	{
		BIND_RPC_METHOD(SqliteProxy::Run)
		BIND_RPC_METHOD(SqliteProxy::Inc)
		BIND_RPC_METHOD(SqliteProxy::Update)
		BIND_RPC_METHOD(SqliteProxy::Delete)
		BIND_RPC_METHOD(SqliteProxy::Commit)
		BIND_RPC_METHOD(SqliteProxy::Replace)
		BIND_RPC_METHOD(SqliteProxy::SetIndex)
		BIND_RPC_METHOD(SqliteProxy::InsertOne)
		BIND_RPC_METHOD(SqliteProxy::InsertBatch)


		BIND_RPC_METHOD(SqliteProxy::Func)
		BIND_RPC_METHOD(SqliteProxy::Find)
		BIND_RPC_METHOD(SqliteProxy::Like)
		BIND_RPC_METHOD(SqliteProxy::Count)
		BIND_RPC_METHOD(SqliteProxy::FindPage)
		BIND_RPC_METHOD(SqliteProxy::Distinct)
		LOG_CHECK_RET_FALSE(this->mSqlite = this->GetComponent<SqliteComponent>())
		return true;
	}

	int SqliteProxy::Run(const std::string& sql, json::w::Document& response)
	{
		LOG_ERROR_CHECK_ARGS(!sql.empty())
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		if (sqliteResponse == nullptr)
		{
			return XCode::Failure;
		}
		if (sqliteResponse->ok)
		{
			response.Add("error", sqliteResponse->error);
			return XCode::Failure;
		}
		response.Add("count", sqliteResponse->count);
		std::unique_ptr<json::w::Value> jsonArray = response.AddArray("list");
		for (const std::unique_ptr<json::r::Document>& result: sqliteResponse->result)
		{
			jsonArray->Push(*result);
		}
		return XCode::Ok;
	}

	int SqliteProxy::Inc(const json::r::Document& request, json::w::Document& response)
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
		this->mFactory.GetTable(tab).Select({ field }).Filter(filter);
		const std::string selectSql = this->mFactory.Limit(1).ToString();
		std::unique_ptr<sqlite::Response> incResponse = this->mSqlite->Run(incSql);
		if (incResponse == nullptr || !incResponse->ok)
		{
			return XCode::Failure;
		}

		std::unique_ptr<sqlite::Response> queryResponse = this->mSqlite->Run(selectSql);
		if (queryResponse == nullptr || !queryResponse->ok)
		{
			return XCode::Failure;
		}

		if (!queryResponse->ok)
		{
			response.Add("error", queryResponse->error);
			return XCode::Failure;
		}
		if (!queryResponse->result.empty())
		{
			std::unique_ptr<json::r::Document>& document = queryResponse->result.front();
			{
				long long number = 0;
				document->Get(field.c_str(), number);
				response.Add(field.c_str(), number);
			}
		}
		return XCode::Ok;
	}

	int SqliteProxy::InsertOne(const json::r::Document& request, json::w::Document& response)
	{
		std::string tab;
		json::r::Value document;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("document", document))
		this->mFactory.GetTable(tab).Insert(document);
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		if (!sqliteResponse->ok)
		{
			response.Add("error", sqliteResponse->error);
			return XCode::Failure;
		}
		response.Add("count", sqliteResponse->count);
		return XCode::Ok;
	}

	int SqliteProxy::InsertBatch(const json::r::Document& request, json::w::Document& response)
	{
		int count = 0;
		std::string tab;
		json::r::Value documents;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("documents", documents))
		{
			json::r::Value document;
			this->mSqlite->StartTransaction();
			for (size_t index = 0; index < documents.MemberCount(); index++)
			{
				LOG_ERROR_CHECK_ARGS(documents.Get(index, document))
				LOG_ERROR_CHECK_ARGS(document.GetType() == YYJSON_TYPE_OBJ)
				std::string sql = this->mFactory.GetTable(tab).Insert(document).ToString();
				std::unique_ptr<sqlite::Response> insertResponse = this->mSqlite->Run(sql);
				if (!insertResponse->ok && insertResponse->count <= 0)
				{
					this->mSqlite->RollbackTransaction();
					return XCode::Failure;
				}
				count += insertResponse->count;
			}
			this->mSqlite->CommitTransaction();
		}
		response.Add("count", count);
		return XCode::Ok;
	}

	int SqliteProxy::Replace(const json::r::Document& request, json::w::Document& response)
	{
		std::string tab;
		json::r::Value document;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("document", document))

		this->mFactory.GetTable(tab).Insert(document);
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		if (!sqliteResponse->ok)
		{
			response.Add("error", sqliteResponse->error);
			return XCode::Failure;
		}
		response.Add("count", sqliteResponse->count);
		return XCode::Ok;
	}

	int SqliteProxy::Delete(const json::r::Document& request, json::w::Document& response)
	{
		std::string tab;
		json::r::Value filter;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("filter", filter))

		int limit = 0;
		this->mFactory.GetTable(tab).Delete().Filter(filter);
		if (request.Get("limit", limit))
		{
			this->mFactory.Limit(limit);
		}
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		if (!sqliteResponse->ok)
		{
			response.Add("error", sqliteResponse->error);
			return XCode::Failure;
		}
		response.Add("count", sqliteResponse->count);
		return XCode::Ok;
	}

	int SqliteProxy::Update(const json::r::Document& request, json::w::Document& response)
	{
		std::string tab;
		json::r::Value filter;
		json::r::Value document;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("filter", filter))
		LOG_ERROR_CHECK_ARGS(request.Get("document", document) && document.IsObject())

		int limit = 1;
		this->mFactory.GetTable(tab).Update(document).Filter(filter);
		if (request.Get("limit", limit))
		{
			this->mFactory.Limit(limit);
		}
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		LOG_ERROR_RETURN_CODE(sqliteResponse != nullptr, XCode::Failure);

		if (!sqliteResponse->ok)
		{
			response.Add("error", sqliteResponse->error);
			return XCode::Failure;
		}
		response.Add("count", sqliteResponse->count);
		return XCode::Ok;
	}

	int SqliteProxy::SetIndex(const json::r::Document& request, json::w::Document& response)
	{
		bool unique = false;
		std::string tab, field;
		request.Get("unique", unique);
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("field", field))

		this->mFactory.GetTable(tab).SetIndex(field, unique);
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		if (!sqliteResponse->ok)
		{
			response.Add("error", sqliteResponse->error);
			return XCode::Failure;
		}
		return XCode::Ok;
	}

	int SqliteProxy::Commit(const json::r::Document& request, json::w::Document& response)
	{
		int okCount = 0;
		size_t count = 0;
		size_t index = 0;
		const char* sql = nullptr;
		this->mSqlite->StartTransaction();
		std::list<std::unique_ptr<json::r::Document>> content;
		do
		{
			sql = request.GetString(index++, count);
			if (sql != nullptr && count > 0)
			{
				std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
				if (!sqliteResponse->ok)
				{
					this->mSqlite->RollbackTransaction();
					return XCode::Failure;
				}
				okCount += sqliteResponse->count;
				for (std::unique_ptr<json::r::Document>& document: sqliteResponse->result)
				{
					content.emplace_back(std::move(document));
				}
			}
		} while (sql != nullptr && count > 0);

		response.Add("count", okCount);
		this->mSqlite->CommitTransaction();
		std::unique_ptr<json::w::Value> jsonArray = response.AddArray("list");
		for (std::unique_ptr<json::r::Document>& document: content)
		{
			jsonArray->Push(*document);
		}
		return XCode::Ok;
	}

	int SqliteProxy::Func(const json::r::Document& request, rpc::Message& response)
	{
		std::string tab, func, field;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab));
		LOG_ERROR_CHECK_ARGS(request.Get("func", func));
		LOG_ERROR_CHECK_ARGS(request.Get("field", field));

		json::r::Value filter;
		this->mFactory.GetTable(tab).Func(func, field, field);
		if(request.Get("filter", filter))
		{
			this->mFactory.Filter(filter);
		}
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		LOG_ERROR_RETURN_CODE(sqliteResponse != nullptr, XCode::Failure);

		if(!sqliteResponse->ok)
		{
			response.SetError(sqliteResponse->error);
			return XCode::Failure;
		}
		if(!sqliteResponse->result.empty())
		{
			std::unique_ptr<json::r::Document> & document = sqliteResponse->result.front();
			{
				size_t count = 0;
				std::unique_ptr<char> buffer;
				if(!document->ToCString(buffer, count))
				{
					return XCode::SerializationFailure;
				}
				response.SetContent(rpc::proto::json, buffer.get(), count);
			}
		}
		return XCode::Ok;
	}

	int SqliteProxy::Find(const json::r::Document& request, rpc::Message& response)
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
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		LOG_ERROR_RETURN_CODE(sqliteResponse != nullptr, XCode::Failure);

		if(!sqliteResponse->ok)
		{
			response.SetError(sqliteResponse->error);
			return XCode::Failure;
		}
		json::w::Document jsonArray(true);
		for(const std::unique_ptr<json::r::Document> & document : sqliteResponse->result)
		{
			jsonArray.Push(*document);
		}
		response.SetContent(jsonArray);
		return XCode::Ok;
	}

	int SqliteProxy::Like(const json::r::Document& request, rpc::Message& response)
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
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		LOG_ERROR_RETURN_CODE(sqliteResponse != nullptr, XCode::Failure);

		if(!sqliteResponse->ok)
		{
			response.SetError(sqliteResponse->error);
			return XCode::Failure;
		}
		json::w::Document jsonArray(true);
		for(const std::unique_ptr<json::r::Document> & result : sqliteResponse->result)
		{
			jsonArray.Push(*result);
		}
		response.SetContent(jsonArray);
		return XCode::Ok;
	}

	int SqliteProxy::FindOne(const json::r::Document& request, rpc::Message& response)
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
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		LOG_ERROR_RETURN_CODE(sqliteResponse != nullptr, XCode::Failure);

		if(!sqliteResponse->ok)
		{
			response.SetError(sqliteResponse->error);
			return XCode::Failure;
		}
		if(!sqliteResponse->result.empty())
		{
			std::unique_ptr<json::r::Document> & document = sqliteResponse->result.front();
			{
				size_t count = 0;
				std::unique_ptr<char> buffer;
				if(!document->ToCString(buffer, count))
				{
					return XCode::SerializationFailure;
				}
				response.SetContent(rpc::proto::json, buffer.get(), count);
			}
		}
		return XCode::Ok;
	}

	int SqliteProxy::FindPage(const json::r::Document& request, rpc::Message& response)
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
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		LOG_ERROR_RETURN_CODE(sqliteResponse != nullptr, XCode::Failure);

		if(!sqliteResponse->ok)
		{
			response.SetError(sqliteResponse->error);
			return XCode::Failure;
		}
		json::w::Document document(true);
		for(const std::unique_ptr<json::r::Document> & result : sqliteResponse->result)
		{
			document.Push(*result);
		}
		response.SetContent(document);
		return XCode::Ok;
	}

	int SqliteProxy::Count(const json::r::Document& request, rpc::Message & response)
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
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		LOG_ERROR_RETURN_CODE(sqliteResponse != nullptr, XCode::Failure);

		if(!sqliteResponse->ok)
		{
			response.SetError(sqliteResponse->error);
			return XCode::Failure;
		}
		if(!sqliteResponse->result.empty())
		{
			std::unique_ptr<json::r::Document> & document = sqliteResponse->result.front();
			{
				size_t count = 0;
				std::unique_ptr<char> buffer;
				if(!document->ToCString(buffer, count))
				{
					return XCode::SerializationFailure;
				}
				response.SetContent(rpc::proto::json, buffer.get(), count);
			}
		}
		return XCode::Ok;
	}

	int SqliteProxy::Distinct(const json::r::Document& request, rpc::Message& response)
	{
		std::string tab;
		json::r::Value filter;
		json::r::Value jsonFields;
		LOG_ERROR_CHECK_ARGS(request.Get("tab", tab))
		LOG_ERROR_CHECK_ARGS(request.Get("filter", filter))
		LOG_ERROR_CHECK_ARGS(request.Get("fields", jsonFields))
		LOG_ERROR_CHECK_ARGS(filter.IsObject() && jsonFields.IsArray())
		this->mFactory.GetTable(tab).Distinct(jsonFields).Filter(filter);
		const std::string sql = this->mFactory.ToString();
		std::unique_ptr<sqlite::Response> sqliteResponse = this->mSqlite->Run(sql);
		if(!sqliteResponse->ok)
		{
			response.SetError(sqliteResponse->error);
			return XCode::Failure;
		}
		json::w::Document jsonArray(true);
		for (const std::unique_ptr<json::r::Document>& result: sqliteResponse->result)
		{
			jsonArray.Push(*result);
		}
		response.SetContent(jsonArray);
		return XCode::Ok;
	}
}