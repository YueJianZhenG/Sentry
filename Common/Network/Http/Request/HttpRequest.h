//
// Created by zmhy0073 on 2021/11/1.
//

#ifndef GameKeeper_HTTPLOCALREQUEST_H
#define GameKeeper_HTTPLOCALREQUEST_H
#include <Network/Http/HttpHandlerBase.h>
namespace GameKeeper
{
	class HttpLocalSession;
    class HttpClientComponent;
    class HttpRequest : public HttpHandlerBase
    {
    public:
        HttpRequest(HttpClientComponent * component);

		~HttpRequest() override;

    public:

        const std::string & GetError() { return this->mError;}

        const std::string &GetVersion() { return this->mVersion; }

        HttpStatus GetHttpCode() const { return (HttpStatus) mHttpCode; }

    public:

        XCode StartHttpRequest(const std::string & url);

        void SetCode(XCode code);

		void OnWriterAfter(XCode code) override;

        void OnReceiveBodyAfter(XCode code) override;

        void OnReceiveHeardAfter(XCode code) override;

		

        bool OnReceiveHeard(asio::streambuf &buf) override;

    protected:
        std::string mHost;
        std::string mPort;
        std::string mPath;
        unsigned int mCorId;
    protected:
        XCode mCode;
        int mHttpCode;
        std::string mError;
        std::string mVersion;
		HttpLocalSession * mLocalSession;
        HttpClientComponent * mHttpComponent;
    };
}
#endif //GameKeeper_HTTPLOCALREQUEST_H
