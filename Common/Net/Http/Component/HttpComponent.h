
#pragma once
#ifdef __ENABLE_OPEN_SSL__
#include"asio/ssl.hpp"
#endif
#include "Rpc/Component/RpcComponent.h"
namespace http
{
	class Content;
	class Request;
	class Response;
	class Client;
}
#ifdef __ENABLE_OPEN_SSL__
#include "Yyjson/Object/JsonObject.h"
namespace ssl
{
	struct Config : public json::Object<Config>
	{
		std::string pem;
	};
}
#endif

namespace acs
{
	class HttpComponent final : public RpcComponent<http::Response>,
								public IRpc<http::Request, http::Response>, public IServerRecord
	{
	public:
		HttpComponent();
	public:
		std::unique_ptr<http::Response> Get(const std::string& url, int second = 15);
		std::unique_ptr<http::Response> Post(const std::string& url, const std::string& data, int second = 15);
		std::unique_ptr<http::Response> Post(const std::string& url, const json::w::Document & json, int second = 15);
	public:
		std::unique_ptr<http::Response> Do(std::unique_ptr<http::Request> & request);
		std::unique_ptr<http::Response> Do(std::unique_ptr<http::Request> & request, std::unique_ptr<http::Content> content);
	public:
		std::unique_ptr<http::Content> Run(std::unique_ptr<http::Request> & request);
		std::unique_ptr<http::Content> Run(std::unique_ptr<http::Request> & request, std::unique_ptr<http::Content> content);
	public:
		int Send(std::unique_ptr<http::Request>& request, int & taskId); // 异步发送
		int Send(std::unique_ptr<http::Request>& request, std::unique_ptr<http::Response>& response, int & taskId); // 异步发送
		int Send(std::unique_ptr<http::Request>& request, std::function<void(std::unique_ptr<http::Response>&)> && cb);
	private:
		bool Awake() final;
		bool LateAwake() final;
		void OnRecord(json::w::Document &document) final;
		std::shared_ptr<http::Client> CreateClient(http::Request * request);
		void OnMessage(int taskId, http::Request *request, http::Response *response) noexcept final;
	private:
		class ThreadComponent * mThread;
#ifdef __ENABLE_OPEN_SSL__
		asio::ssl::context mSslContext;
		std::unordered_map<std::string, asio::ssl::context *> mSslContexts;
#endif
		std::unordered_map<int, std::shared_ptr<http::Client>> mUseClients;
	};
}