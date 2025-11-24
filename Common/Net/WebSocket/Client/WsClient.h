//
// Created by 64658 on 2025/1/2.
//

#ifndef APP_WS_CLIENT_H
#define APP_WS_CLIENT_H
#include "Network/Tcp/Client.h"
#include "Entity/Component/IComponent.h"
#include "WebSocket/Common/WebSocketMessage.h"

namespace http
{
	class Request;
	class Response;
}
namespace ws
{
	typedef acs::IRpc<rpc::Message, rpc::Message> Component;
	class Client final : public tcp::Client
	{
	public:
		Client(int id, Component * component, Asio::Context & main, char msg);
	public:
		void Close();
		void Send(std::unique_ptr<ws::Message>& message);
		void Send(std::unique_ptr<rpc::Message> & message);
	private:
		void OnUpdate() final;
		void OnSendMessage(size_t size) final;
		void OnReadError(const Asio::Code &code) final;
		void OnSendMessage(const Asio::Code &code) final;
		void OnConnect(const Asio::Code &code, int count) final;
		void OnReceiveLine(std::istream &readStream, size_t size) final;
		void OnReceiveMessage(std::istream &readStream, size_t size, const asio::error_code &code) final;
	private:
		void Close(int code);
		void SendFirstMessage();
		bool OnMessage(const ws::Message & message);
		void AddToSendQueue(std::unique_ptr<ws::Message>& message);
	private:
		char mMsg;
		int mSockId;
		ws::Message mMessage;
		Component * mComponent;
		std::stringstream mStream;
		Asio::Context & mMainContext;
		http::Request * mHttpRequest;
		http::Response * mHttpResponse;
		std::queue<std::unique_ptr<ws::Message>> mWaitSendMessage;
	};
}



#endif //APP_WS_CLIENT_H
