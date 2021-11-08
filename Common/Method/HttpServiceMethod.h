//
// Created by zmhy0073 on 2021/11/2.
//

#ifndef GAMEKEEPER_HTTPSERVICEMETHOD_H
#define GAMEKEEPER_HTTPSERVICEMETHOD_H
#include <Network/Http/Http.h>
#include <Util/JsonHelper.h>
#include <Network/Http/Content/HttpReadContent.h>
#include <Network/Http/HttpRemoteSession.h>
#include <Network/Http/Response/HttpRequestHandler.h>
#include<Network/Http/Content/HttpWriteContent.h>
namespace GameKeeper
{
    class HttpRequestHandler;

    template<typename T>
    using HttpServiceMethodType = HttpStatus(T::*)(HttpRequestHandler *);

    template<typename T>
    using HttpServiceJsonMethodType = XCode(T::*)(const RapidJsonReader & request, RapidJsonWriter & response);

    class HttpServiceMethod
    {
    public:
        virtual HttpStatus Invoke(HttpRequestHandler *handler) = 0;
    };

    template<typename T>
    class HttpServiceMethod1 : public HttpServiceMethod
    {
    public:
        HttpServiceMethod1(T *o, HttpServiceMethodType<T> func)
                : _o(o), _func(func){}

    public:
        HttpStatus Invoke(HttpRequestHandler *handler) override
        {
            return (_o->*_func)(handler);
        }
    private:
        T *_o;
        HttpServiceMethodType<T> _func;
    };
    template<typename T>
    class HttpServiceJsonMethod : public HttpServiceMethod
    {
    public:
        HttpServiceJsonMethod(T * o, HttpServiceJsonMethodType<T> func)
                : _o(o), _func(func){}

    public:
        HttpStatus Invoke(HttpRequestHandler *handler) override
        {

            return HttpStatus::OK;
        }
    private:
        T * _o;
        HttpServiceJsonMethodType<T> _func;
    };
}
#endif //GAMEKEEPER_HTTPSERVICEMETHOD_H
