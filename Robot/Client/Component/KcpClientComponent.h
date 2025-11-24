//
// Created by 64658 on 2025/9/10.
//

#ifndef APP_KCPCLIENTCOMPONENT_H
#define APP_KCPCLIENTCOMPONENT_H

#include "Kcp/Common/KcpClient.h"
#include "Entity/Component/Component.h"
namespace acs
{
	class KcpClientComponent final : public Component, public ISystemUpdate,
			public IRpc<rpc::Message, rpc::Message>, public rpc::IInnerSender
	{
	public:
		KcpClientComponent();
	public:
		void Remove(int id) final;
		int Connect(const std::string & address) final;
		char GetNet() const noexcept final { return rpc::net::client; }
		int Send(int id, std::unique_ptr<rpc::Message> &message) noexcept final;
	private:
		bool Awake() final;
		bool LateAwake() final;
		void OnSystemUpdate(long long now) noexcept final;
		int OnRequest(std::unique_ptr<rpc::Message>& request);
		void OnMessage(rpc::Message *request, rpc::Message *response) noexcept final;
	private:
		int mIndex;
		class ProtoComponent * mProto;
		class LuaComponent * mLuaComponent;
		class DispatchComponent * mDisComponent;
		std::unordered_map<int, std::shared_ptr<kcp::Client>> mClients;
	};
}


#endif //APP_KCPCLIENTCOMPONENT_H
