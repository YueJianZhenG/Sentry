//
// Created by 64658 on 2025/3/4.
//

#ifndef APP_TELNETCOMPONENT_H
#define APP_TELNETCOMPONENT_H
#include "Event/Base/IEvent.h"
#include "Telnet/Client/TelnetClient.h"
#include "Entity/Component/Component.h"
#include "Server/Component/ITcpComponent.h"

namespace help
{
	DEFINE_STATIC_EVENT(OnTelnetMessageEvent, telnet::Request *, telnet::Response *);
}

namespace acs
{
	class TelnetComponent : public Component, public IRpc<telnet::Request, telnet::Response>, public ITcpListen
	{
	public:
		TelnetComponent();
	private:
		bool LateAwake() final;
		void OnClientError(int id, int code) final;
		bool OnListen(tcp::Socket *socket) noexcept final;
		void OnMessage(int id, telnet::Request *request, telnet::Response *response) noexcept final;
	private:
		class RouterComponent * mRouter;
		math::NumberPool<int> mNumberPool;
		class EventProxyComponent * mEventProxy;
		std::unordered_map<int, std::shared_ptr<telnet::Client>> mClients;
	};
}


#endif //APP_TELNETCOMPONENT_H
