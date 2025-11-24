//
// Created by 64658 on 2024/10/24.
//

#pragma once
#include "asio.hpp"
#include "Rpc/Common/Message.h"
#include "Kcp/Common/KcpSession.h"
#include "Entity/Component/Component.h"
#include "Server/Component/ITcpComponent.h"
using asio_udp = asio::ip::udp;

namespace acs
{
	class OuterKcpComponent final : public Component, public rpc::IInnerSender
	                           , public ISystemUpdate, public INetListen, public IRpc<rpc::Message, rpc::Message>
	{
	public:
		OuterKcpComponent();
		~OuterKcpComponent() final = default;
	private:
		bool LateAwake() final;
		bool StopListen() final;
		void OnSystemUpdate(long long now) noexcept final;
		bool StartListen(const acs::ListenConfig &listen) final;
	private:
		void Remove(int id) final;
		kcp::Session * GetSession();
		void GetAddress(std::string & address);
		kcp::Session * GetSession(const std::string & address);
		void OnMessage(int id, rpc::Message *request, rpc::Message *response) noexcept final;
	public:
		char GetNet() const noexcept final { return rpc::net::kcp; }
		int Send(int id, std::unique_ptr<rpc::Message> & message) noexcept final;
		void Broadcast(std::unique_ptr<rpc::Message> &message) noexcept final;
	private:
		void StartReceive();
		bool OnReceiveMessage(size_t size);
		int OnRequest(std::unique_ptr<rpc::Message> & message);
	private:
		std::string mDecodeBuffer;
		asio::mutable_buffers_1 mRecvBuf;
		char mRecvBuffer[kcp::BUFFER_COUNT];
		std::unique_ptr<asio_udp::socket> mKcpServer;
	private:
		char mMsg;
		ListenConfig mConfig;
		asio::streambuf mReceiveBuffer;
		asio_udp::endpoint mRemotePoint;
		class DispatchComponent * mDispatch;
		math::NumberPool<int> mNumberFactory;
		std::unordered_map<int, std::shared_ptr<kcp::Session>> mSessions;
		std::unordered_map<std::string, std::weak_ptr<kcp::Session>> mClients;
	};
}
