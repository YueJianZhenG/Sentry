//
// Created by zmhy0073 on 2022/8/29.
//

#ifndef APP_SERVERWEB_H
#define APP_SERVERWEB_H
#include"LocalHttpService.h"
namespace Sentry
{
    class ServerWeb : public LocalHttpService
    {
    public:
        ServerWeb() = default;
        ~ServerWeb() final = default;
    private:     
        bool OnInit() final;
    private:
        int Info(Json::Writer & response);
		int Hotfix(Json::Writer& response);
		int Ping(const Http::Request& request, Http::Response& response);
		int Hello(const Http::Request& request, Http::Response& response);
		int DownLoad(const Http::Request& request, Http::Response& response);
		int Sleep(const Json::Reader & request, Json::Writer& response);
    };
}


#endif //APP_SERVERWEB_H
