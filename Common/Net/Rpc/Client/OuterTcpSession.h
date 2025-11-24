//
// Created by mac on 2021/11/28.
//

#ifndef GAMEKEEPER_RPCPROXYCLIENT_H
#define GAMEKEEPER_RPCPROXYCLIENT_H
#include"Rpc/Common/Message.h"
#include"Network/Tcp/Client.h"
#include"Entity/Component/IComponent.h"

namespace rpc
{
    // 网关session
 	class OuterTcpSession final : public tcp::Client
#ifdef __SHARE_PTR_COUNTER__
			, public memory::Object<OuterTcpSession>
#endif
	{
	 public:
		typedef acs::IRpc<rpc::Message, rpc::Message> Component;
		explicit OuterTcpSession(int id, Component* component, Asio::Context & main);
		~OuterTcpSession() final;
	 public:
		void Stop();
		void Send(std::unique_ptr<rpc::Message> & message);
		void StartReceive(tcp::Socket * socket, int second = 0);
	protected:
		void OnReadError(const Asio::Code &code) final;
        void OnReceiveMessage(std::istream & readStream, size_t, const Asio::Code & code) final;
	private:
		void SendFirstMessage();
		void CloseSocket(int code);
		void OnSendMessage(size_t size) final;
		void OnMessage(std::unique_ptr<rpc::Message> & message);
		void OnSendMessage(const asio::error_code &code) final;
 		void AddToSendQueue(std::unique_ptr<rpc::Message> & message);
	private:
		int mRecvCount;
		int mDecodeState;
		const int mSockId;
		Component * mComponent;
		long long mLastRecvTime;
		rpc::ProtoHead mProtoHead;
		Asio::Context & mMainContext;
		std::unique_ptr<rpc::Message> mMessage;
		std::queue<std::unique_ptr<rpc::Message>> mSendMessages;
		std::list<std::pair<std::string, std::string>> mParameter;
	};
}


#endif //GAMEKEEPER_RPCPROXYCLIENT_H
