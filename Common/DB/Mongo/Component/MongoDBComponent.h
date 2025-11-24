//
// Created by mac on 2022/6/28.
//

#ifndef APP_MONGODBCOMPONENT_H
#define APP_MONGODBCOMPONENT_H
#include"Mongo/Client/MongoProto.h"
#include"Mongo/Client/MongoClient.h"
#include"Proto/Document/BsonDocument.h"
#include"Mongo/Config/MongoConfig.h"
#include"Core/Queue/Queue.h"
#include"Rpc/Component/RpcComponent.h"

namespace acs
{
	class MongoTask final : public IRpcTask<mongo::Response>, protected WaitTaskSourceBase
	{
	public:
		explicit MongoTask(int taskId);
    public:
		void OnTimeout() final;
		inline std::unique_ptr<mongo::Response> Await();
		inline void OnResponse(std::unique_ptr<mongo::Response>& response) noexcept final;
	private:
		std::unique_ptr<mongo::Response> mMessage;
	};

	inline std::unique_ptr<mongo::Response> MongoTask::Await()
	{
		this->YieldTask();
		return std::move(this->mMessage);
	}

	inline void MongoTask::OnResponse(std::unique_ptr<mongo::Response>& response) noexcept
	{
		this->mMessage = std::move(response);
		this->ResumeTask();
	}

	inline void MongoTask::OnTimeout()
	{
		bson::w::Document document;
		document.Add("ok", "0.0");
		document.Add("errmsg", "time out");

		int count = 0;
		const char * str = document.Serialize(count);
		this->mMessage->buffer = std::make_unique<char[]>(count);
		for(int index = 0; index < count; index++)
		{
			this->mMessage->buffer[index] = str[index];
		}
		this->mMessage = std::make_unique<mongo::Response>("");
		this->mMessage->document.Init(this->mMessage->buffer.get());
	}
}

namespace acs
{
	class LuaMongoTask final : public IRpcTask<mongo::Response>
	{
	public:
		LuaMongoTask(lua_State * lua, int id);
		~LuaMongoTask() final;
	public:
		int Await() noexcept;
		void OnTimeout() final;
		void OnResponse(std::unique_ptr<mongo::Response>& response) noexcept final;
	private:
		int mRef;
		lua_State * mLua;
	};

}

namespace acs
{
	class MongoDBComponent final : public RpcComponent<mongo::Response>, public ISecondUpdate,
			public IRpc<mongo::Request, mongo::Response>, public IDestroy, public IServerRecord
	{
	public:
		MongoDBComponent();
		~MongoDBComponent() final = default;
	public:
		bool FormatUri(std::string & uri);
		void Send(std::unique_ptr<mongo::Request> & request, int & taskId);
		std::unique_ptr<mongo::Response> Run(std::unique_ptr<mongo::Request> & request);
		std::unique_ptr<mongo::Response> Run(const std::string & db, std::unique_ptr<mongo::Request> & request);
	private:
		bool Awake() final;
		bool LateAwake() final;
		void OnDestroy() final;
		void OnConnectOK(int id) final;
		void OnClientError(int id, int code) final;
		void OnSecondUpdate(int tick) noexcept final;
		void OnRecord(json::w::Document &document) final;
		void OnSendFailure(int id, mongo::Request *message) final;
		void OnExplain(std::unique_ptr<mongo::Request> & request, long long ms) noexcept;
		void OnMessage(int id, mongo::Request * request, mongo::Response * message) noexcept final;
	private:
		void AddFreeClient(int id);
		void Send(std::unique_ptr<mongo::Request> request);
		void Send(int id, std::unique_ptr<mongo::Request> request);
		static bool DecodeUrl(const std::string & url, mongo::Config & config);
	private:
		int mRetryCount;
		mongo::Cluster mConfig;
		unsigned long long mSumCount;
		custom::Queue<int> mFreeClients;
		std::unordered_set<int> mRetryClients; //断开了 重试的客户端
		std::queue<std::unique_ptr<mongo::Request>> mRequests;
		std::unordered_map<int, std::shared_ptr<mongo::Client>> mClients;
	};
}


#endif //APP_MONGODBCOMPONENT_H
