
#pragma once
#include<Component/Component.h>
#include <Network/Http/HttpSessionBase.h>
namespace Sentry
{
    class HttpClientComponent : public Component, public SocketHandler<HttpSessionBase>
    {
    public:
        HttpClientComponent()
        {}

        ~HttpClientComponent()
        {}

    protected:
         SessionBase * CreateSocket() override { return nullptr; }
         void OnCloseSession(HttpSessionBase * session) override;
         bool OnReceiveMessage(HttpSessionBase * session, const string & message) override;
		 void OnListenNewSession(HttpSessionBase * session, const asio::error_code & err) override;
    public:
        bool Awake() final;
        void Start() final;
    public:
        XCode Get(const std::string &url, std::string &json, int timeout = 5);

    private:
        class TaskPoolComponent *mTaskComponent;
        class CoroutineComponent *mCorComponent;
    };
}