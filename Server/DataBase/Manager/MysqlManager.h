#pragma once
#include<XCode/XCode.h>
#include<Manager/Manager.h>
#include<MysqlClient/MysqlDefine.h>
#include<QueryResult/InvokeResultData.h>
namespace SoEasy
{
	class MysqlTaskAction;
	class MysqlManager : public Manager
	{
	public:
		MysqlManager();
		~MysqlManager() { }
	public:
		SayNoMysqlSocket * GetMysqlSocket(long long threadId);
		const std::string & GetDataBaseName() { return this->mDataBaseName; }
	public:
		shared_ptr<InvokeResultData> InvokeCommand(const std::string & sql);
	public:
		bool InsertData(const std::string tab, shared_ptr<Message> data);
		bool QueryData(const std::string tab, shared_ptr<Message> data, const std::string & key);
		bool UpdateData(const std::string tab, shared_ptr<Message> data, const std::string & key);
		bool DeleteData(const std::string tab, shared_ptr<Message> data, const std::string & key);
	protected:
		bool OnInit() final;
		void OnInitComplete() final;
		void PushClassToLua(lua_State * luaEnv) final;		//������������lua

	private:
		bool StartConnectMysql();
	private:
		std::string mMysqlIp;		//ip��ַ
		unsigned short mMysqlPort;	//�˿ں�
		std::string mDataBaseUser;	//�û���
		std::string mDataBasePasswd; //����
		std::string mDataBaseName;	//���ݿ�����
		ThreadPool * mThreadPool;		//�̳߳�
		std::string mSqlTablePath;
		SayNoMysqlSocket * mMysqlSockt;
		std::unordered_map<long long, SayNoMysqlSocket *> mMysqlSocketMap;	//�߳�id�� socket
	private:
		class CoroutineManager * mCoroutineManager;
	};
}