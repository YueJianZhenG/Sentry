//
// Created by zmhy0073 on 2021/11/1.
//

#ifndef GameKeeper_HTTPLOCALPOSTREQUEST_H
#define GameKeeper_HTTPLOCALPOSTREQUEST_H
#include "HttpLocalRequest.h"
#include <Network/Http/Content/HttpWriteContent.h>
namespace GameKeeper
{
    class HttpLocalPostRequest : public HttpLocalRequest
    {
    public:
        explicit HttpLocalPostRequest(HttpClientComponent * component);
        ~HttpLocalPostRequest() override = default;
    public:
        XCode Post(const std::string & url, HttpWriteContent & content , std::string & response);
    protected:
        bool WriterToBuffer(std::ostream & os) override;
        void OnReceiveBody(asio::streambuf & buf) override;
    private:
        unsigned int mCorId;
        std::string * mResponse;
        HttpWriteContent * mPostContent;
    };
}
#endif //GameKeeper_HTTPLOCALPOSTREQUEST_H
