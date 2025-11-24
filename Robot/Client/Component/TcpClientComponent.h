//
// Created by leyi on 2023/9/11.
//

#ifndef APP_TCPCLIENTCOMPONENT_H
#define APP_TCPCLIENTCOMPONENT_H
#include"Rpc/Common/Message.h"
#include"Rpc/Client/InnerTcpClient.h"
#include"Entity/Component/Component.h"

namespace acs
{
	class TcpClientComponent final : public Component,
			public IRpc<rpc::Message, rpc::Message>, public rpc::IInnerSender
	{
	public:
		TcpClientComponent();
	private:
		bool Awake() final;
		bool LateAwake() final;
		void OnClientError(int id, int code) final;
		void OnSendFailure(int id, rpc::Message *message) final;
		void OnMessage(rpc::Message *request, rpc::Message *response) noexcept final;
	private:
		void Remove(int id) final;
		int Connect(const std::string & address) final;
		int Send(int id, std::unique_ptr<rpc::Message>& message) noexcept final;
		char GetNet() const noexcept final { return rpc::net::client; }
	private:
		int OnRequest(std::unique_ptr<rpc::Message>& request);
	private:
		int mIndex;
		class ProtoComponent * mProto;
		class LuaComponent * mLuaComponent;
		class DispatchComponent * mDisComponent;
		std::queue<std::unique_ptr<rpc::InnerTcpClient>> mClientQueue;
		std::unordered_map<int, std::shared_ptr<rpc::InnerTcpClient>> mClientMap;

	};
}


#endif //APP_TCPCLIENTCOMPONENT_H
