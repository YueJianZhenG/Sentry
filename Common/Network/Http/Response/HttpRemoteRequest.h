//
// Created by zmhy0073 on 2021/11/1.
//

#ifndef SENTRY_HTTPREMOTEREQUEST_H
#define SENTRY_HTTPREMOTEREQUEST_H
#include <Network/Http/HttpHandlerBase.h>

namespace Sentry
{
    class HttpRemoteSession;

    class HttpClientComponent;

    class HttpRemoteRequest : public HttpHandlerBase
    {
    public:
        HttpRemoteRequest(HttpClientComponent *component, HttpRemoteSession *session);

        ~HttpRemoteRequest() override = default;

    public:
        HttpRemoteSession * GetSession() { return this->mHttpSession; }
    protected:
        bool OnSessionError(const asio::error_code &code) override;

        bool OnReceiveHeard(asio::streambuf &buf, size_t size) override;

    protected:
        std::string mPath;
        std::string mVersion;
        HttpRemoteSession *mHttpSession;
        HttpClientComponent *mHttpComponent;
    };
}

#endif //SENTRY_HTTPREMOTEREQUEST_H
