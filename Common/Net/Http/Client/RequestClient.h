//
// Created by yjz on 2022/1/19.
//

#ifndef APP_REQUESTCLIENT_H
#define APP_REQUESTCLIENT_H
#include"Http.h"
#include"Network/Tcp/TcpClient.h"
#include"Http/Common/HttpRequest.h"
#include"Http/Common/HttpResponse.h"
#include"Entity/Component/IComponent.h"

namespace http
{
	class RequestClient : public tcp::TcpClient
	{
	 public:
		typedef joke::IRpc<Request, Response> Component;
		explicit RequestClient(Component * component);
	 public:
		void Do(std::unique_ptr<http::Request> request, std::unique_ptr<http::Response> response, int taskId);
	 private:
        void OnComplete(HttpStatus code);
		void OnTimeout(tcp::TimeoutFlag flag) final;
		void OnConnect(bool result, int count) final;
		void OnReadError(const Asio::Code &code) final;
        void OnReceiveLine(std::istream &is, size_t) final;
        void OnReceiveMessage(std::istream & is, size_t) final;
	private:
		void OnSendMessage() final;
		void OnSendMessage(const asio::error_code &code) final;
	 private:
		Component * mComponent;
        std::unique_ptr<http::Request> mRequest;
		std::unique_ptr<http::Response> mResponse;
    };
}
#endif //APP_REQUESTCLIENT_H
