//
// Created by yjz on 2022/1/19.
//

#ifndef GAMEKEEPER_HTTPREQUESTCLIENT_H
#define GAMEKEEPER_HTTPREQUESTCLIENT_H
#include"Network/SocketProxy.h"
#include<Async/TaskSource.h>
#include"HttpAsyncRequest.h"
namespace Sentry
{
	class HttpRequestClient : std::enable_shared_from_this<HttpRequestClient>
	{
	 public:
		HttpRequestClient(std::shared_ptr<SocketProxy> socketProxy);
	 public:
		std::shared_ptr<HttpAsyncResponse> Get(const std::string& url);
		std::shared_ptr<HttpAsyncResponse> Post(const std::string& url, Json::Writer& json);
		std::shared_ptr<HttpAsyncResponse> Request(std::shared_ptr<HttpAsyncRequest> request);
		std::shared_ptr<HttpAsyncResponse> Post(const std::string& url, const std::string& content);
	private:
		void SendByStream(std::shared_ptr<IHttpStream> httpStream);
		void ReceiveHttpContent( std::shared_ptr<IHttpContent> httpContent);
		void ConnectHost(const std::string& host, const std::string& port);
	 private:
		std::string mUrl;
		asio::streambuf mReadBuffer;
		std::shared_ptr<SocketProxy> mSocket;
		std::shared_ptr<TaskSource<bool>> mReadTask;
		std::shared_ptr<TaskSource<bool>> mWriteTask;
		std::shared_ptr<TaskSource<bool>> mConnectTask;
	};
}
#endif //GAMEKEEPER_HTTPREQUESTCLIENT_H
