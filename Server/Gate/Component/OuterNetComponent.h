//
// Created by mac on 2021/11/28.
//

#ifndef GAMEKEEPER_GATECLIENTCOMPONENT_H
#define GAMEKEEPER_GATECLIENTCOMPONENT_H
#include<queue>
#include<unordered_map>
#include<unordered_set>
#include"Server/Component/TcpListenerComponent.h"

namespace Rpc
{
    class Packet;
}
namespace Tendo
{

	class OuterNetClient;
    class OuterNetComponent : public TcpListenerComponent, public IRpc<Rpc::Packet>,
							  public IComplete, public IServerRecord, public IFrameUpdate, public IDestroy
	{
	 public:
		OuterNetComponent() = default;
		~OuterNetComponent() final = default;
	 public:
		void OnTimeout(const std::string &address) final;
		void StartClose(const std::string & address) final;
		void OnMessage(std::shared_ptr<Rpc::Packet> message) final;
		void OnCloseSocket(const std::string & address, int code) final;
    public:
		bool StopClient(long long userId);
		bool BindClient(const std::string & address, long long userId);
		size_t Broadcast(const std::shared_ptr<Rpc::Packet> & message);
		int OnRequest(long long userId, std::shared_ptr<Rpc::Packet> & message);
		bool Send(long long userId, const std::shared_ptr<Rpc::Packet> & message);
		bool Send(const std::string & address, const std::shared_ptr<Rpc::Packet> & message);
	private:
		bool Awake() final;
		bool LateAwake() final;
        void OnDestroy() final;
        void OnClusterComplete() final;
		void OnFrameUpdate(float t) final;
	 private:
        void OnRecord(Json::Writer & document) final;
        void OnListen(std::shared_ptr<SocketProxy> socket) final;
		bool GetUserId(const std::string & address, long long & userId) const;
	private:
		int mMaxHandlerCount;
		unsigned int mSumCount;
		unsigned int mWaitCount;
		class RpcService * mGateService;
		class NodeMgrComponent * mNodeComponent;
#ifdef __DEBUG__
		std::unordered_map<int, long long> mRecords;
#endif
		std::queue<std::shared_ptr<Rpc::Packet>> mMessages;
		class InnerNetMessageComponent * mInnerMessageComponent;
        std::queue<std::shared_ptr<OuterNetClient>> mClientPools;
		std::unordered_map<std::string, long long> mAddressUserMap; //验证过的客户端
		std::unordered_map<long long, std::string> mUserAddressMap; //验证过的客户端
		std::unordered_map<std::string, std::shared_ptr<OuterNetClient>> mGateClientMap;
	};
}


#endif //GAMEKEEPER_GATECLIENTCOMPONENT_H
