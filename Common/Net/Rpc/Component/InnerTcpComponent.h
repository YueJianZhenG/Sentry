#pragma once
#include"Rpc/Common/Message.h"
#include"Rpc/Client/InnerTcpClient.h"
#include"Server/Component/ListenerComponent.h"
struct lua_State;
namespace acs
{
	// 管理内网rpc的session
	class InnerTcpComponent : public Component, public ITcpListen,
							  public IRpc<rpc::Message, rpc::Message>, public IServerRecord, public rpc::IInnerSender
	{
	 public:
		InnerTcpComponent();
		~InnerTcpComponent() override = default;
	 public:
		void StartClose(int id) final;
		void OnSendFailure(int id, rpc::Message * message) final;
		void OnMessage(rpc::Message * message, rpc::Message *) noexcept final ;
	private:
		char GetNet()  const noexcept final { return rpc::net::tcp; }
		int Send(int id, std::unique_ptr<rpc::Message> & message) noexcept final;
		void Broadcast(std::unique_ptr<rpc::Message> &message) noexcept final;
	protected:
        bool LateAwake() final;
		void OnClientError(int id, int code) final;
		void OnRecord(json::w::Document & document) final;
		bool OnListen(tcp::Socket * socket) noexcept final;
	private:
		rpc::InnerTcpClient * GetClient(int id);
		int OnForward(std::unique_ptr<rpc::Message> & message) noexcept;
	private:
		unsigned long long mWaitCount;
		class NodeComponent * mActor;
		math::NumberPool<int> mNumPool;
		class ThreadComponent * mThread;
        class DispatchComponent* mDispatch;
		std::unordered_map<int, std::shared_ptr<rpc::InnerTcpClient>> mClients; //本地客户端(连接别的)
		std::unordered_map<int, std::shared_ptr<rpc::InnerTcpClient>> mSessions; //本地客户端(连接别的)
	};
}
