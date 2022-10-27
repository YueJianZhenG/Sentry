//
// Created by yjz on 2022/10/27.
//

#ifndef APP_HTTPRESPONSE_H
#define APP_HTTPRESPONSE_H
#include"Comman.h"
namespace Http
{
    class Response : public IStream
    {
    public:
        Response();
    public:
        int OnRead(std::istream &buffer) final;
        int OnWrite(std::ostream &buffer) final;
    public:
        void Json(HttpStatus code, const std::string & json);
        void Json(HttpStatus code, const char * str, size_t len);
    public:
        HttpStatus Code() const { return (HttpStatus)this->mCode; }
        const std::string & GetError() const { return this->mError; }
        const std::string & Content() const { return this->mContent; }
    private:
        Head mHead;
        int mCode;
        State mParseState;
        std::string mError;
        std::string mVersion;
        std::string mContent;
    };
}


#endif //APP_HTTPRESPONSE_H
