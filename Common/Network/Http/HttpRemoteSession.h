//
// Created by zmhy0073 on 2021/10/26.
//

#ifndef SENTRY_HTTPREMOTESESSION_H
#define SENTRY_HTTPREMOTESESSION_H
#include "HttpSessionBase.h"
namespace Sentry
{
    class HttpHandler;
    class HttpClientComponent;
    class HttpRemoteSession : public HttpSessionBase
    {
    public:
        HttpRemoteSession(HttpClientComponent * socketHandler);
    public:
        SocketType GetSocketType() override { return SocketType::RemoteSocket;}
    protected:
        bool WriterToBuffer(std::ostream &os) override;
        bool OnReceiveBody(asio::streambuf &buf, const asio::error_code &code) override;
        bool OnReceiveHeard(asio::streambuf &buf,size_t size, const asio::error_code &code) override;
    private:
        int mReadCount;
        bool mIsReadBody;
        std::string mPath;
        std::string mMethod;
        std::string mVersion;
        HttpHandler * mHttpHandler;
        HttpClientComponent * mHttpComponent;
    };
}

#endif //SENTRY_HTTPREMOTESESSION_H
