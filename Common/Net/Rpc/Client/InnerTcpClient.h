#pragma once
#include "Rpc/Common/Message.h"
#include "Network/Tcp/Client.h"
#include "Entity/Component/IComponent.h"
using namespace tcp;

namespace rpc
{
	class InnerTcpClient : public tcp::Client
#ifdef __SHARE_PTR_COUNTER__
			, public memory::Object<InnerTcpClient>
#endif
	{
	 public:
		typedef acs::IRpc<rpc::Message, rpc::Message> Component;
		explicit InnerTcpClient(int id, Component * component, bool client, Asio::Context &);
		~InnerTcpClient() final;
	public:
		void Close();
		void StartReceive(tcp::Socket * socket);
		bool Send(std::unique_ptr<rpc::Message> & message);
	 private:
		void CloseSocket();
		void CloseSocket(int code);
		void OnSendMessage(size_t size) final;
		bool MakeMessage(const rpc::ProtoHead & header);
		void OnReadError(const Asio::Code &code) final;
		void OnSendMessage(const Asio::Code & code) final;
		void OnConnect(const Asio::Code &code, int count) final;
		void AddToSendQueue(std::unique_ptr<rpc::Message> & message);
		void OnReceiveMessage(std::istream & is, size_t, const Asio::Code & code) final;
	private:
		const int mSockId;
		int mDecodeStatus;
		const bool mIsClient;
		Component * mComponent;
		rpc::ProtoHead mProtoHead;
		Asio::Context & mMainContext;
		std::unique_ptr<rpc::Message> mMessage;
		std::queue<std::unique_ptr<rpc::Message>> mSendMessages;
		std::unordered_map<int, std::unique_ptr<rpc::Message>> mWaitResMessages; //等待返回的服务器消息
	};
}// namespace Sentry