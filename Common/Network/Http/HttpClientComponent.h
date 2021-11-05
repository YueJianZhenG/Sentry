
#pragma once
#include<Component/Component.h>
#include <Network/Http/HttpSessionBase.h>
namespace GameKeeper
{
    class HttpWriteContent;
    class HttpRemoteSession;
    class HttpServiceMethod;
    class HttpRequestHandler;
    class HttpClientComponent : public Component, public ISocketListen
    {
    public:
        HttpClientComponent() = default;

        ~HttpClientComponent() final = default;
    public:
        bool Awake() final;

        void Start() final;

    public:
        XCode Get(const std::string &url, std::string &json, int timeout = 5);

        XCode Download(const std::string & url, const std::string & path, int timeout = 5);

        XCode Post(const std::string &url, const std::string &data, std::string &response, int timeout = 5);

        XCode Post(const std::string &url, const std::unordered_map<std::string, std::string> &data, std::string &response,
             int timeout = 5);

        XCode Post(const std::string &url, HttpWriteContent &content, std::string &response, int timeout);

    public:
        void OnListen(SocketProxy *socket) final;

        void HandlerHttpRequest(HttpRequestHandler *remoteRequest);

        HttpRequestHandler *CreateMethodHandler(const std::string &method, HttpRemoteSession *session);

        HttpServiceMethod * GetHttpMethod(const std::string & service, const std::string & method);
    private:

        void Invoke(HttpRequestHandler *remoteRequest);

    private:
        class CoroutineComponent *mCorComponent;
    };
}