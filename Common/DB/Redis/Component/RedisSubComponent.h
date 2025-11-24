//
// Created by leyi on 2024/1/18.
//

#ifndef APP_REDISSUBCOMPONENT_H
#define APP_REDISSUBCOMPONENT_H
#include "Redis/Client/RedisClient.h"
#include "Redis/Config/RedisConfig.h"
#include "Rpc/Component/RpcComponent.h"


namespace acs
{
	class RedisSubComponent final : public RpcComponent<redis::Response>,
									public IRpc<redis::Request, redis::Response>, public ISecondUpdate
	{
	public:
		RedisSubComponent();
		~RedisSubComponent() final = default;
	public:
		bool Sub(const std::string & chanel);
		bool UnSub(const std::string & chanel);
		void Send(std::unique_ptr<redis::Request>& request, int & rpcId);
	private:
		bool Awake() final;
		bool LateAwake() final;
		void OnConnectOK(int id) final;
		void OnClientError(int id, int code) final;
		void OnSecondUpdate(int tick) noexcept final;
		void OnMessage(int, redis::Request *request, redis::Response *response) noexcept final;
	private:
		bool mIsSend; //是否正在发送
		redis::Cluster mConfig;
		class TimerComponent * mTimer;
		std::shared_ptr<redis::Client> mClient;
		std::unordered_set<std::string> mChannels;
		std::queue<std::unique_ptr<redis::Request>> mMessages;
	};
}


#endif //APP_REDISSUBCOMPONENT_H
