//
// Created by mac on 2021/11/28.
//

#ifndef GAMEKEEPER_GATECLIENTCOMPONENT_H
#define GAMEKEEPER_GATECLIENTCOMPONENT_H

#include "Core/Queue/Queue.h"
#include "Rpc//Client/OuterTcpSession.h"
#include "Entity/Component/Component.h"
#include "Server/Component/ITcpComponent.h"

namespace rpc
{
    class Message;
	class InnerTcpClient;
}

namespace acs
{
	class OuterTcpComponent final : public Component, public ITcpListen, public rpc::IOuterSender,
									public IRpc<rpc::Message, rpc::Message>, public IServerRecord, public IFrameUpdate
	{
	 public:
		OuterTcpComponent();
		~OuterTcpComponent() final = default;
	public:
		void StartClose(int id, int code) final;
	private:
		bool LateAwake() final;
		void OnFrameUpdate(int elapse) noexcept final;
		void Broadcast(std::unique_ptr<rpc::Message> & message) noexcept final;
		int Send(int id, std::unique_ptr<rpc::Message> & message) noexcept final;
		inline char GetNet() const noexcept final { return rpc::net::tcp; }
	private:
		void OnClientError(int id, int code) final;
		void OnSendFailure(int id, rpc::Message *message) final;
		void OnMessage(rpc::Message * request, rpc::Message *) noexcept final;
	private:
		bool OnListen(tcp::Socket * socket) noexcept final;
		void OnRecord(json::w::Document & document) final;
	private:
		int mWaitCount;
		int mMaxConnectCount;
		rpc::IOuterMessage * mOuter;
		math::NumberPool<int> mSocketPool;
		std::queue<std::unique_ptr<rpc::Message>> mBroadCastMessages;
		std::unordered_map<int, std::shared_ptr<rpc::OuterTcpSession>> mSessions;
		//std::unordered_map<int, std::unique_ptr<rpc::InnerClient>> mForwardClientMap;
	};
}


#endif //GAMEKEEPER_GATECLIENTCOMPONENT_H
