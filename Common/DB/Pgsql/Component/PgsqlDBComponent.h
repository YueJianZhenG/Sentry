//
// Created by 64658 on 2025/2/18.
//

#ifndef APP_PGSQLDBCOMPONENT_H
#define APP_PGSQLDBCOMPONENT_H
#include "DB/Common/TableInfo.h"
#include "Util/File/FileFactory.h"
#include "Pgsql/Client/PgsqlClient.h"
#include "Rpc/Component/RpcComponent.h"

namespace acs
{
	class PgsqlTask final : public IRpcTask<pgsql::Response>, protected WaitTaskSourceBase
	{
	public:
		explicit PgsqlTask(int taskId) :  IRpcTask<pgsql::Response>(taskId) { }
	public:
		inline void OnTimeout() final;
		inline std::unique_ptr<pgsql::Response> Await();
		inline void OnResponse(std::unique_ptr<pgsql::Response>& response) noexcept final;
	private:
		std::unique_ptr<pgsql::Response> mMessage;
	};
	inline std::unique_ptr<pgsql::Response> PgsqlTask::Await()
	{
		this->YieldTask();
		return std::move(this->mMessage);
	}

	inline void PgsqlTask::OnTimeout()
	{
		this->mMessage = std::make_unique<pgsql::Response>();
		this->mMessage->error.emplace_back("time out");
		this->ResumeTask();
	}

	inline void PgsqlTask::OnResponse(std::unique_ptr<pgsql::Response>& response) noexcept
	{
		this->mMessage = std::move(response);
		this->ResumeTask();
	}
}

namespace acs
{
	class PgsqlDBComponent  : public RpcComponent<pgsql::Response>, public IServerRecord,
							  public IRpc<pgsql::Request, pgsql::Response>, public ISecondUpdate, public IStart, public IDestroy
	{
	public:
		PgsqlDBComponent();
	public:
		void Send(std::unique_ptr<pgsql::Request>& request);
		void Send(std::unique_ptr<pgsql::Request>& request, int & rpcId);
	public:
		std::unique_ptr<pgsql::Response> Run(const std::string & sql);
		std::unique_ptr<pgsql::Response> Run(std::unique_ptr<pgsql::Request>& request);
	private:
		void AddFreeClient(int id);
		void Send(int id, std::unique_ptr<pgsql::Request>& request);
		static bool DecodeUrl(const std::string & url, pgsql::Config & config);
	private:
		bool InitDataBase();
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
		void OnSendFailure(int id, pgsql::Request *message) final;
		void OnExplain(const std::string & sql, long long ms) noexcept;
		void OnMessage(int, pgsql::Request *request, pgsql::Response *response) noexcept final;
	private:
		int mRetryCount;
		pgsql::Cluster mConfig;
		unsigned long long mSumCount;
		help::FileFactory mFileFactory;
		custom::Queue<int> mFreeClients;
		std::unordered_set<int> mRetryClients; //断开了 重试的客户端
		std::queue<std::unique_ptr<pgsql::Request>> mMessages;
		std::unordered_map<int, std::shared_ptr<pgsql::Client>> mClients;
	};
}


#endif //APP_PGSQLDBCOMPONENT_H
