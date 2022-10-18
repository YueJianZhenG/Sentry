//
// Created by mac on 2021/11/28.
//

#ifndef GAMEKEEPER_GATECLIENTCOMPONENT_H
#define GAMEKEEPER_GATECLIENTCOMPONENT_H

#include"Client/Message.h"
#include"Component/Component.h"
#include"Component/TcpListenerComponent.h"
namespace Sentry
{

	class OuterNetClient;
	class OuterNetComponent : public TcpListenerComponent, public IRpc<Rpc::Packet>
	{
	 public:
		OuterNetComponent() = default;
		~OuterNetComponent() final = default;
	 public:
		void StartClose(const std::string & address) final;
		void OnCloseSocket(const std::string & address, XCode code) final;
        void OnMessage(const std::string &address, std::shared_ptr<Rpc::Packet> message) final;
    public:
        bool IsAuth(const std::string & address);
        bool SendData(std::shared_ptr<Rpc::Packet> message);
		OuterNetClient* GetGateClient(const std::string& address);
        bool SendData(long long userId, std::shared_ptr<Rpc::Packet> message);
        bool SendData(const std::string & address, std::shared_ptr<Rpc::Packet> message);
	 public:
		bool Awake() final;
		bool LateAwake() final;
	 private:
        void OnStopListen() final;
        bool OnListen(std::shared_ptr<SocketProxy> socket) final;
        bool OnAuth(const std::string & address, std::shared_ptr<Rpc::Packet> message);
        bool OnRequest(const std::string & address, std::shared_ptr<Rpc::Packet> message);
    private:
		class TimerComponent* mTimerComponent;
        class NetThreadComponent * mNetComponent;
        class LocationComponent * mLocationComponent;
        std::unordered_set<std::string> mAuthClients; //已经验证过的客户端
        class OuterNetMessageComponent* mOuterMessageComponent;
        std::queue<std::shared_ptr<OuterNetClient>> mClientPools;
		std::unordered_map<std::string, std::shared_ptr<OuterNetClient>> mGateClientMap;
	};
}


#endif //GAMEKEEPER_GATECLIENTCOMPONENT_H
