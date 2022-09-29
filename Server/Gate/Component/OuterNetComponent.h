//
// Created by mac on 2021/11/28.
//

#ifndef GAMEKEEPER_GATECLIENTCOMPONENT_H
#define GAMEKEEPER_GATECLIENTCOMPONENT_H
#include"Component/Component.h"
#include"Client/Message.h"
#include"Listener/TcpServerListener.h"
namespace Sentry
{
	class OuterNetClient;
	class OuterNetComponent : public Component, public TcpServerListener,
                              public IRpc<Rpc::Data>
	{
	 public:
		OuterNetComponent() = default;
		~OuterNetComponent() final = default;
	 public:
		void StartClose(const std::string & address) final;
		void OnCloseSocket(const std::string & address, XCode code) final;
        void OnMessage(const std::string &address, std::shared_ptr<Rpc::Data> message) final;
    public:
		bool GetUserAddress(long long userId, std::string & address);
        bool GetUserId(const std::string & address, long long & userId);
        std::shared_ptr<OuterNetClient> GetGateClient(const std::string & address);
	 public:
        std::string CreateToken(long long userId, float second = 10);
        void SendToAllClient(std::shared_ptr<c2s::rpc::call> message);
		bool SendData(const std::string & address, std::shared_ptr<Rpc::Data> message);
	 public:
		void Awake() final;
		bool LateAwake() final;
		bool OnListen(std::shared_ptr<SocketProxy> socket) final;
	 private:
        bool StartInComplete() final { return false; }
        bool OnRequest(const std::string & address, std::shared_ptr<Rpc::Data> message);
    private:
		class TimerComponent* mTimerComponent;
        class NetThreadComponent * mNetComponent;
        class RedisDataComponent * mRedisComponent;
        std::unordered_map<std::string, long long> mTokens;
        class OuterNetMessageComponent* mOuterMessageComponent;
        std::queue<std::shared_ptr<OuterNetClient>> mClientPools;
        std::unordered_map<std::string, long long> mUserAddressMap;
        std::unordered_map<long long, std::string> mClientAddressMap;
		std::unordered_map<std::string, std::shared_ptr<OuterNetClient>> mGateClientMap;
	};
}


#endif //GAMEKEEPER_GATECLIENTCOMPONENT_H
