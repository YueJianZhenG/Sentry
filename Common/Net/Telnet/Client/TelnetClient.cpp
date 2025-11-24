//
// Created by 64658 on 2025/3/4.
//

#include "TelnetClient.h"
#include "XCode/XCode.h"

namespace telnet
{
	Client::Client(int id, tcp::Socket* socket, Asio::Context& main, Component * component)
		: tcp::Client(socket, 0), mSocketId(id), mMain(main), mComponent(component)
	{

	}

	void Client::StartReceive()
	{
		Asio::Context & context = this->mSocket->GetContext();
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(context, [self, this]() { this->ReadLine(); });
	}

	void Client::StartClose()
	{
		Asio::Context & context = this->mSocket->GetContext();
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(context, [self, this]() { this->mSocket->Close(); });
	}

	void Client::Send(std::unique_ptr<telnet::Response> message)
	{
		Asio::Context & context = this->mSocket->GetContext();
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(context, [self, this, req = message.release()]()
		{
			this->Write(*req);
			this->mResponse.reset(req);
		});
	}

	void Client::OnSendMessage(size_t size)
	{
		this->ReadLine();
		this->mResponse.reset();
	}

	void Client::OnSendMessage(const Asio::Code& code)
	{
		this->StopTimer();
		this->mSocket->Close();
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(this->mMain, [self, this]()
		{
			int id = this->mSocketId;
			this->mComponent->OnClientError(id, XCode::NetSendFailure);
		});
	}

	void Client::OnReadError(const Asio::Code& code)
	{
		this->StopTimer();
		this->mSocket->Close();
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(this->mMain, [self, this]()
		{
			int id = this->mSocketId;
			this->mComponent->OnClientError(id, XCode::NetReadFailure);
		});
	}

	void Client::OnReceiveLine(std::istream& readStream, size_t size)
	{
		std::unique_ptr<telnet::Request> request = std::make_unique<telnet::Request>();
		std::unique_ptr<telnet::Response> response = std::make_unique<telnet::Response>();
		{
			request->OnRecvMessage(readStream, size);
			if(request->GetCmd().empty())
			{
				this->ReadLine();
				return;
			}
			this->mComponent->OnMessage(request.get(), response.get());
			this->Send(std::move(response));
		}
	}
}