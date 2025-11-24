//
// Created by 64658 on 2025/1/2.
//

#ifndef APP_OuterWsComponent_H
#define APP_OuterWsComponent_H
namespace ws
{
	class Message;
	class Session;
	class Client;
}

#include "Rpc/Component/RpcComponent.h"
#include "Server/Component/ITcpComponent.h"

namespace acs
{
	class OuterWsComponent final : public ITcpListen, public rpc::IOuterSender, public IServerRecord,
								   public RpcComponent<rpc::Message>, public IRpc<rpc::Message, rpc::Message>, public IFrameUpdate
	{
	public:
		OuterWsComponent();
	public:
		bool LateAwake() final;
		void OnClientError(int id, int code) final;
		void OnFrameUpdate(int elapse) noexcept final;
		bool OnListen(tcp::Socket *socket) noexcept final;
		void OnMessage(rpc::Message *request, rpc::Message *response) noexcept final;
	public:
		void Broadcast(std::unique_ptr<rpc::Message> &message) noexcept final;
		int Send(int id, std::unique_ptr<rpc::Message> &message) noexcept final;
		inline char GetNet() const noexcept final { return rpc::net::ws; }
	private:
		void OnRecord(json::w::Document &document) final;
	private:
		unsigned int mSumCount;
		rpc::IOuterMessage * mOuter;
		math::NumberPool<int> mNumPool;
		std::queue<std::unique_ptr<rpc::Message>> mBroadCastMessages;
		std::unordered_map<int, std::shared_ptr<ws::Session>> mSessions;
	};
}


#endif //APP_OuterWsComponent_H
