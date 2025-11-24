//
// Created by 64658 on 2025/6/6.
//

#ifndef APP_SQLITEPROXY_H
#define APP_SQLITEPROXY_H
#include "DB/Common/SqlFactory.h"
#include "Rpc/Service/RpcService.h"


namespace acs
{
	class SqliteProxy final : public RpcService
	{
	public:
		SqliteProxy();
		~SqliteProxy() final = default;
	private:
		bool OnInit() final;
	private:
		int Run(const std::string & sql, json::w::Document & response);
		int Inc(const json::r::Document & request, json::w::Document & response);
		int Commit(const json::r::Document & request, json::w::Document & response);
		int Replace(const json::r::Document & request, json::w::Document & response);
		int Delete(const json::r::Document & request, json::w::Document & response);
		int Update(const json::r::Document & request, json::w::Document & response);
		int SetIndex(const json::r::Document & request, json::w::Document & response);
		int InsertOne(const json::r::Document & request, json::w::Document & response);
		int InsertBatch(const json::r::Document & request, json::w::Document & response);
	private:
		int Func(const json::r::Document & request, rpc::Message & response);
		int Like(const json::r::Document & request, rpc::Message & response);
		int Find(const json::r::Document & request, rpc::Message & response);
		int Count(const json::r::Document & request, rpc::Message & response);
		int FindOne(const json::r::Document & request, rpc::Message & response);
		int FindPage(const json::r::Document & request, rpc::Message & response);
		int Distinct(const json::r::Document & request, rpc::Message & response);
	private:
		sql::Factory mFactory;
		class SqliteComponent * mSqlite;
	};
}



#endif //APP_SQLITEPROXY_H
