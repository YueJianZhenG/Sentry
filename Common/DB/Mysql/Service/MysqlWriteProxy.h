//
// Created by 64658 on 2025/6/6.
//

#ifndef APP_MYSQLWRITEPROXY_H
#define APP_MYSQLWRITEPROXY_H
#include "DB/Common/SqlFactory.h"
#include "Rpc/Service/RpcService.h"


namespace acs
{
	class MysqlWriteProxy final : public RpcService
	{
	public:
		MysqlWriteProxy();
		~MysqlWriteProxy() final = default;
	private:
		bool OnInit() final;
	public:
		int Run(const std::string & sql, json::w::Document & response);
		int Inc(const json::r::Document & request, json::w::Document & response);
		int Commit(const json::r::Document & request, json::w::Document & response);
		int Replace(const json::r::Document & request, json::w::Document & response);
		int Delete(const json::r::Document & request, json::w::Document & response);
		int Update(const json::r::Document & request, json::w::Document & response);
		int Execute(const json::r::Document & request, rpc::Message & response); //执行预处理sql
		int SetIndex(const json::r::Document & request, json::w::Document & response);
		int InsertOne(const json::r::Document & request, json::w::Document & response);
		int InsertBatch(const json::r::Document & request, json::w::Document & response);
		int Deduction(const json::r::Document & request, json::w::Document & response);
	private:
		sql::Factory mFactory;
		class MysqlDBComponent * mMysql;
	};
}



#endif //APP_MYSQLWRITEPROXY_H
