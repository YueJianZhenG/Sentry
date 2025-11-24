#pragma once

#include"Util/Tools/Guid.h"
#include"Core/Queue/Queue.h"
#include"Redis/Client/RedisClient.h"
#include"Redis/Config/RedisConfig.h"
#include"Rpc/Component/RpcComponent.h"

namespace acs
{
	struct RedisLuaData
	{
		std::string name;
		std::string json;
	};
}

namespace acs
{
	class ThreadComponent;

	class RedisComponent final : public RpcComponent<redis::Response>, public IStart,
								 public IRpc<redis::Request, redis::Response>, public IDestroy, public IServerRecord, public ISecondUpdate
	{
	 public:
		RedisComponent();
    public:
		bool Ping();
		void Send(std::unique_ptr<redis::Request>& request);
		void Send(std::unique_ptr<redis::Request>& request, int & id);
	public:
		bool UnLock(const std::string & key);
		bool Lock(const std::string & key, int timeout = 5);
	public:
        template<typename ... Args>
        void Send(const std::string & cmd, Args&& ... args) noexcept;
        template<typename ... Args>
		std::unique_ptr<redis::Response> Run(const std::string & cmd, Args&& ... args) noexcept;
	public:
		bool Send(const RedisLuaData & data) noexcept;
		bool Send(const RedisLuaData & data, int & taskId, unsigned int & timeout) noexcept;
	public:
		std::unique_ptr<redis::Response> CallLua(const RedisLuaData & data);
		std::unique_ptr<json::r::Document> Call(const RedisLuaData & data);
	public:
		bool Del(const std::string & key);
		bool LoadRedisScript(const std::string & dir);
		const redis::Cluster & GetConfig() const { return this->mConfig; }
		std::unique_ptr<redis::Response> Run(std::unique_ptr<redis::Request> & request) noexcept;
		bool MakeLuaRequest(const RedisLuaData & data, std::unique_ptr<redis::Request>& request);
	private:
		void Send(int id, std::unique_ptr<redis::Request> & request) noexcept;
		void OnMessage(int id, redis::Request * request, redis::Response * response) noexcept final;
	private:
		void AddFreeClient(int id);
		static bool DecodeUrl(const std::string & url, redis::Config & config);
		bool OnLoadScript(const std::string & name, const std::string &md5);
	private:
        bool Awake() final;
		void OnStart() final;
		void OnDestroy() final;
        bool LateAwake() final;
		void OnConnectOK(int id) final;
		void OnClientError(int id, int code) final;
		void OnSecondUpdate(int tick) noexcept final;
		void OnRecord(json::w::Document &document) final;
		void OnSendFailure(int id, redis::Request *message) final;
	private:
		int mRetryCount;
		redis::Cluster mConfig;
		unsigned long long mSumCount;
		custom::Queue<int> mFreeClients; //空闲客户端
		std::shared_ptr<redis::Client> mSubClient;
		std::unordered_set<int> mRetryClients;
		std::unordered_map<std::string, std::string> mLuaMap;
		std::queue<std::unique_ptr<redis::Request>> mRequests;
		std::unordered_map<int, std::shared_ptr<redis::Client>> mClients;
	};

    template<typename ... Args>
	std::unique_ptr<redis::Response> RedisComponent::Run(const std::string & cmd, Args &&...args) noexcept
    {
        std::unique_ptr<redis::Request> request = std::make_unique<redis::Request>();
		{
			request->SetCommand(cmd);
			redis::Request::InitParameter(request.get(), std::forward<Args>(args)...);
		}
        return this->Run(request);
    }

    template<typename ... Args>
	void RedisComponent::Send(const std::string &cmd, Args &&...args) noexcept
    {
		std::unique_ptr<redis::Request> request = std::make_unique<redis::Request>();
		{
			request->SetCommand(cmd);
			redis::Request::InitParameter(request.get(), std::forward<Args>(args)...);
		}
		this->Send(request);
    }
}