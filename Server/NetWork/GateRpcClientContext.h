//
// Created by mac on 2021/11/28.
//

#ifndef GAMEKEEPER_RPCPROXYCLIENT_H
#define GAMEKEEPER_RPCPROXYCLIENT_H

#include"Network/Rpc/RpcClientContext.h"
using namespace google::protobuf;
namespace Sentry
{
	class GateClientComponent;
	class GateRpcClientContext : public RpcClientContext
	{
	 public:
		GateRpcClientContext(std::shared_ptr<SocketProxy> socket, SocketType type, GateClientComponent* component);
		~GateRpcClientContext() final = default;

	 public:
		void StartClose();
		unsigned int GetQps() const
		{
			return this->mQps;
		}
		bool SendToClient(std::shared_ptr<c2s::Rpc::Call> message);
		bool SendToClient(std::shared_ptr<c2s::Rpc_Response> message);
		unsigned int GetCallCount() const
		{
			return this->mCallCount;
		}
	 protected:
		void OnConnect(XCode code) final;
		void OnClientError(XCode code) final;
		bool OnReceiveMessage(char type, const char *buffer, size_t size) final;
		void OnSendData(XCode code, std::shared_ptr<NetworkData> message) final;
	 private:
		unsigned int mQps;
		unsigned int mCallCount;
		GateClientComponent* mGateComponent;
	};
}


#endif //GAMEKEEPER_RPCPROXYCLIENT_H