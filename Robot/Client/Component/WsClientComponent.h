//
// Created by leyi on 2023/9/11.
//

#ifndef APP_WSCLIENTCOMPONENT_H
#define APP_WSCLIENTCOMPONENT_H
#include"Rpc/Common/Message.h"
#include"Entity/Component/Component.h"
#include"WebSocket/Client/WsClient.h"

namespace acs
{
	class WsClientComponent final : public Component,
									 public IRpc<rpc::Message, rpc::Message>, public rpc::IInnerSender
	{
	public:
		WsClientComponent();
	private:
		bool Awake() final;
		bool LateAwake() final;
		void OnClientError(int id, int code) final;
		void OnSendFailure(int id, rpc::Message *message) final;
		void OnMessage(rpc::Message *request, rpc::Message *response) noexcept final;
	private:
		void Remove(int id) final;
		int Connect(const std::string & address) final;
		inline char GetNet() const noexcept final { return rpc::net::client; }
		int Send(int id, std::unique_ptr<rpc::Message> & message) noexcept final;
	private:
		int OnRequest(std::unique_ptr<rpc::Message> & request);
	private:
		int mIndex;
		class ProtoComponent * mProto;
		class LuaComponent * mLuaComponent;
		class DispatchComponent * mDisComponent;
		std::unordered_map<int, std::shared_ptr<ws::Client>> mClientMap;

	};
}


#endif //APP_WSCLIENTCOMPONENT_H
