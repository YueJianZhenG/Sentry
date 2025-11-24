//
// Created by 64658 on 2025/3/4.
//

#include "TelnetComponent.h"
#include "Entity/Actor/App.h"
#include "Server/Config/CodeConfig.h"
#include "Router/Component/RouterComponent.h"
#include "Event/Component/EventProxyComponent.h"
namespace acs
{
	TelnetComponent::TelnetComponent()
	{
		this->mRouter = nullptr;
		this->mEventProxy = nullptr;
	}

	bool TelnetComponent::LateAwake()
	{
		this->mRouter = this->GetComponent<RouterComponent>();
		this->mEventProxy = this->GetComponent<EventProxyComponent>();
		return true;
	}

	bool TelnetComponent::OnListen(tcp::Socket* socket) noexcept
	{
		int id = this->mNumberPool.BuildNumber();
		Asio::Context & main = this->mApp->GetContext();
		std::shared_ptr<telnet::Client> client = std::make_shared<telnet::Client>(id, socket, main, this);
		{
			const std::string msg("===== welcome connect acs server =====");
			std::unique_ptr<telnet::Response> welcome = std::make_unique<telnet::Response>(msg);
			{
				client->Send(std::move(welcome));
				this->mClients.emplace(id, client);
			}
		}
		return true;
	}

	void TelnetComponent::OnClientError(int id, int code)
	{
		auto iter = this->mClients.find(id);
		if(iter != this->mClients.end())
		{
			this->mClients.erase(iter);
		}
	}

	void TelnetComponent::OnMessage(int id, telnet::Request* request, telnet::Response* response) noexcept
	{
		std::string eventId = fmt::format("on_{}", request->GetCmd());
		if(this->mEventProxy->Trigger(eventId, request, response) < 0)
		{

		}
	}
}