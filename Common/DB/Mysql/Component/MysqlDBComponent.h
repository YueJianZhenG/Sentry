
#pragma once

#include "DB/Common/TableInfo.h"
#include "Util/File/FileFactory.h"
#include "Mysql/Client/MysqlClient.h"
#include "Mysql/Common/MysqlProto.h"
#include "Rpc/Component/RpcComponent.h"

namespace acs
{
	class MysqlTask final : public IRpcTask<mysql::Response>, protected WaitTaskSourceBase
	{
	public:
		explicit MysqlTask(int taskId) :  IRpcTask<mysql::Response>(taskId) { }
	public:
		void OnTimeout() final;
		inline std::unique_ptr<mysql::Response> Await();
		inline void OnResponse(std::unique_ptr<mysql::Response>& response) noexcept final;
	private:
		std::unique_ptr<mysql::Response> mMessage;
	};
	inline std::unique_ptr<mysql::Response> MysqlTask::Await()
	{
		this->YieldTask();
		return std::move(this->mMessage);
	}

	inline void MysqlTask::OnTimeout()
	{
		this->mMessage = std::make_unique<mysql::Response>();
		{
			this->mMessage->error.emplace_back("time out");
		}
		this->ResumeTask();
	}

	inline void MysqlTask::OnResponse(std::unique_ptr<mysql::Response>& response) noexcept
	{
		this->mMessage = std::move(response);
		this->ResumeTask();
	}
}

namespace acs
{
	class MysqlDBComponent : public RpcComponent<mysql::Response>, public IServerRecord, public IStart,
							 public IRpc<mysql::Request, mysql::Response>, public IDestroy, public ISecondUpdate
	{
	public:
		MysqlDBComponent();
	public:
		void Send(std::unique_ptr<mysql::Request>& request);
		void Send(std::unique_ptr<mysql::Request>& request, int & rpcId);
	public:
		std::unique_ptr<mysql::Response> Run(const std::string & sql);
		std::unique_ptr<json::r::Document> Invoke(const std::string & sql);
		std::unique_ptr<mysql::Response> Run(std::unique_ptr<mysql::Request> & request);
	private:
		void AddFreeClient(int id);
		void Send(int id, std::unique_ptr<mysql::Request>& request);
		static bool DecodeUrl(const std::string & url, mysql::Config & config);
	private:
		bool InitDataBaseAndSession();
		bool InitTable(const std::string & name, const sql::Table & tableInfo);
		bool CreateTable(const std::string & name, const sql::Table & tableInfo);
	private:
		bool Awake() final;
		void OnStart() final;
		bool LateAwake() final;
		void OnDestroy() final;
		void OnConnectOK(int id) final;
		void OnClientError(int id, int code) final;
		void OnSecondUpdate(int tick) noexcept final;
		void OnRecord(json::w::Document &document) final;
		void OnSendFailure(int id, mysql::Request *message) final;
		void OnExplain(const std::string & sql, long long ms) noexcept;
		void OnMessage(int id, mysql::Request *request, mysql::Response *response) noexcept final;
	private:
		int mRetryCount;
		mysql::Cluster mConfig;
		unsigned long long mCount; //总处理数量
		help::FileFactory mFileFactory;
		custom::Queue<int> mFreeClients; //空闲的客户端
		std::unordered_set<int> mRetryClients; //断开了 重试的客户端
		std::queue<std::unique_ptr<mysql::Request>> mMessages;
		std::unordered_map<std::string, sql::Table> mTableInfos;
		std::unordered_map<int, std::shared_ptr<mysql::Client>> mClients;
	};
}
