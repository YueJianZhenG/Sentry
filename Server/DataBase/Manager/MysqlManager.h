#pragma once
#include <XCode/XCode.h>
#include <Manager/Manager.h>
#include <Util/NumberHelper.h>
#include <MysqlClient/MysqlDefine.h>
#include <QueryResult/InvokeResultData.h>

namespace Sentry
{
	class SqlTableConfig
	{
	public:
		SqlTableConfig(const std::string tab, const std::string pb);

	public:
		void AddKey(const std::string key);
		bool HasKey(const std::string &key);

	public:
		const std::string mTableName;
		std::vector<std::string> mKeys;
		const std::string mProtobufName;
	};
}

namespace Sentry
{
	class MysqlTaskAction;
	class MysqlThreadTask;
	class MysqlManager : public Manager
	{
	public:
		MysqlManager();
		~MysqlManager() {}

	public:
		SayNoMysqlSocket *GetMysqlSocket(long long threadId);
		const std::string &GetDataBaseName() { return this->mDataBaseName; }

	public:
		SqlTableConfig *GetTableConfig(const std::string &tab);
		bool GetTableName(const std::string &pb, std::string &table);
		bool GetTableNameByProtocolName(const std::string & name, std::string & tableName);

	public:
		bool GetAddSqlCommand(const Message & messageData, std::string & sqlCommand);
		bool GetSaveSqlCommand(const Message & messageData, std::string & sqlCommand);
		bool GetQuerySqlCommand(const Message & messageData, std::string & sqlCommand);
		bool GetDeleleSqlCommand(const Message & messageData, std::string & sqlCommand);

	protected:
		bool OnInit() final;
		void OnInitComplete() final;
	private:
		bool InitMysqlTable();
		bool StartConnectMysql();

	private:
		std::string mMysqlIp;		 //ip地址
		unsigned short mMysqlPort;	 //端口号
		std::string mDataBaseUser;	 //用户名
		std::string mDataBasePasswd; //密码
		std::string mDataBaseName;	 //数据库名字
		std::string mSqlTablePath;
		SayNoMysqlSocket *mMysqlSockt;
		std::stringstream mSqlCommandStream;
		std::stringstream mSqlCommandStream2;
		std::unordered_map<std::string, std::string> mTablePbMap;
		std::unordered_map<std::string, SqlTableConfig *> mSqlConfigMap;   //sql表配置
		std::unordered_map<long long, SayNoMysqlSocket *> mMysqlSocketMap; //线程id和 socket
	private:
		class ThreadTaskManager* mTaskManager;
		class CoroutineManager *mCoroutineManager;
	};
}