//
// Created by 64658 on 2025/1/2.
//

#ifndef APP_WS_SESSION_H
#define APP_WS_SESSION_H
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
	class Session final : public tcp::Client
	{
	public:
		Session(int id, Component * component, Asio::Context & main, char msg);
		~Session() final;
	public:
		void Stop();
		void StartReceive(tcp::Socket * tcpSocket);
		void Send(std::unique_ptr<ws::Message>& message);
		void Send(std::unique_ptr<rpc::Message> & message);
	private:
		void OnSendMessage(size_t size) final;
		void OnReadError(const Asio::Code &code) final;
		void OnSendMessage(const Asio::Code &code) final;
		void OnReceiveLine(std::istream &readStream, size_t size) final;
		void OnReceiveMessage(std::istream &readStream, size_t size, const asio::error_code &code) final;
	private:
		void OnPing();
		bool OnMessage();
		bool OnMessage(std::unique_ptr<rpc::Message> & message);
		void AddToSendQueue(std::unique_ptr<ws::Message> & message);
	private:
		void OnReadBody();
		bool DecodeByHttp();
		void Close(int code);
		void StartReceiveWebSocket();
	private:
		char mMsg;
		int mSockId;
		bool mIsHttp;
		Component * mComponent;
		std::stringstream mStream;
		Asio::Context & mMainContext;
		http::Request * mHttpRequest;
		std::unique_ptr<ws::Message> mMessage;
		std::queue<std::unique_ptr<ws::Message>> mWaitSendMessage;
		std::list<std::pair<std::string, std::string>> mParameter;
	};
}



#endif //APP_WS_SESSION_H
