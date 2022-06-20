
#pragma once
#include"Json/JsonWriter.h"
#include"Json/JsonReader.h"
#include"Component/Rpc/RpcTaskComponent.h"
namespace Sentry
{
    class HttpConfig
    {
    public:
        std::string Url;
        std::string Type;
		std::string Content;
		std::string Component;
        std::string MethodName;
    };
}
namespace Sentry
{
	class HttpServiceMethod;
	class HttpHandlerClient;
	class HttpAsyncResponse;
	class HttpHandlerRequest;
	class HttpRequestClient;
    class HttpHandlerResponse;
	class HttpComponent : public RpcTaskComponent<HttpAsyncResponse>,
            public ISocketListen, public ILuaRegister
	{
	 public:
		HttpComponent() = default;
		~HttpComponent() final = default;
	 public:
		void Awake() final;
		bool LateAwake() final;
	 public:
		std::shared_ptr<HttpRequestClient> CreateClient();
		XCode Download(const std::string & url, const std::string & path);
		std::shared_ptr<HttpAsyncResponse> Get(const std::string& url, float second = 15.0f);
		std::shared_ptr<HttpAsyncResponse> Post(const std::string& url, const std::string& data, float second = 15.0f);
	 public:
		void OnListen(std::shared_ptr<SocketProxy> socket) final;
		void OnLuaRegister(Lua::ClassProxyHelper &luaRegister) final;
		void ClosetHttpClient(std::shared_ptr<HttpHandlerClient> httpClient);
	 public:
		void OnRequest(std::shared_ptr<HttpHandlerClient> httpClient);
        TimerComponent * GetTimerComponent() final { return this->mTimeComponent; }
    private:
        const HttpInterfaceConfig * OnHandler(const HttpHandlerRequest & requets, HttpHandlerResponse & response);
	 private:
		TimerComponent * mTimeComponent;
		class TaskComponent* mTaskComponent;
#ifndef ONLY_MAIN_THREAD
		class NetThreadComponent* mThreadComponent;
#endif
		std::unordered_map<std::string, const HttpInterfaceConfig *> mHttpConfigs;
		std::unordered_map<std::string, std::shared_ptr<HttpHandlerClient>> mHttpClients;
	};
}