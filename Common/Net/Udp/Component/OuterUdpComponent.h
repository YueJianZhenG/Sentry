//
// Created by 64658 on 2024/10/24.
//

#pragma once;
#include "asio.hpp"
#include "Rpc/Common/Message.h"
#include "Udp/Client/UdpSession.h"
#include "Entity/Component/Component.h"
#include "Server/Component/ITcpComponent.h"

namespace udp
{
	using Socket = asio::ip::udp::socket;
	using EndPoint = asio::ip::udp::endpoint;

	constexpr unsigned int REMOVE_TIMEOUT = 20; //超时移除时间
	constexpr unsigned int BUFFER_COUNT = 1200;
}

namespace acs
{
	class OuterUdpComponent final : public Component, public rpc::IInnerSender,
									public IRpc<rpc::Message, rpc::Message>, public INetListen
	{
	public:
		OuterUdpComponent();
		~OuterUdpComponent() final = default;
	private:
		bool LateAwake() final;
		bool StopListen() final;
		bool StartListen(const acs::ListenConfig &listen) final;
	private:
		udp::Session * GetSession();
		void GetAddress(std::string & address);
		udp::Session * GetSession(const std::string & address);
		void OnMessage(rpc::Message *request, rpc::Message *response) noexcept final;
	public:
		char GetNet() const noexcept final { return rpc::net::udp; }
		void Broadcast(std::unique_ptr<rpc::Message> &message) noexcept final;
		int Send(int id, std::unique_ptr<rpc::Message> &message) noexcept final;
	private:
		void StartReceive();
		void OnReceiveMessage(size_t size);
		int OnRequest(std::unique_ptr<rpc::Message>& message) noexcept;
	private:
		char mMsg;
		std::istream mReadStream;
		udp::EndPoint mRemotePoint;
		asio::streambuf mReceiveBuffer;
		math::NumberPool<int> mNumPool;
		class DispatchComponent * mDispatch;
		std::unique_ptr<udp::Socket> mSocket;
		std::unordered_map<int, std::shared_ptr<udp::Session>> mSessions;
		std::unordered_map<std::string, std::weak_ptr<udp::Session>> mSessionClients;
	};
}
