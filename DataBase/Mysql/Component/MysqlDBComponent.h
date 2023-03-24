//
// Created by zmhy0073 on 2022/7/16.
//

#ifndef SERVER_MYSQLRPCCOMPONENT_H
#define SERVER_MYSQLRPCCOMPONENT_H

#include"Client/MysqlDefine.h"
#include"Client/MysqlMessage.h"
#include"Guid/NumberBuilder.h"
#include"Component/RpcTaskComponent.h"
struct lua_State;
namespace Sentry
{
    class MysqlTask : public IRpcTask<Mysql::Response>
    {
    public:
        explicit MysqlTask(int taskId);
    public:
        void OnResponse(std::shared_ptr<Mysql::Response> response) final;
        std::shared_ptr<Mysql::Response> Await() { return mTask.Await(); }
    private:
        TaskSource<std::shared_ptr<Mysql::Response>> mTask;
    };
}

namespace Sentry
{
    class MysqlClient;
	class MysqlDBComponent : public RpcTaskComponent<int, Mysql::Response>,
							 public IRpc<Mysql::Response>, public ILuaRegister
    {
    public:
        MysqlDBComponent();
        ~MysqlDBComponent() = default;
    public:
        void CloseClient(int id);
		int MakeMysqlClient();
		bool Ping(int index = 0);
	public:
		bool Execute(int index, const std::shared_ptr<Mysql::ICommand>& command);
		bool Send(int index, const std::shared_ptr<Mysql::ICommand> & command, int & rpcId);
		std::shared_ptr<Mysql::Response> Run(int index, const std::shared_ptr<Mysql::ICommand>& command);
	 private:
		void OnLuaRegister(Lua::ClassProxyHelper &luaRegister) final;
		void OnConnectSuccessful(const std::string &address) final;
		void OnMessage(std::shared_ptr<Mysql::Response> message) final;
		void OnTaskComplete(int key) final { this->mNumberPool.Push(key);}
	private:
		std::unique_ptr<SqlHelper> mSqlHelper;
		Util::NumberBuilder<int, 1> mNumberPool;
		//std::vector<std::unique_ptr<MysqlClient>> mMysqlClients;
		std::unordered_map<int, std::shared_ptr<MysqlClient>> mMysqlClients;
    };
}


#endif //SERVER_MYSQLRPCCOMPONENT_H
