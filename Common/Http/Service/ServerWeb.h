//
// Created by zmhy0073 on 2022/8/29.
//

#ifndef APP_SERVERWEB_H
#define APP_SERVERWEB_H
#include"PhysicalHttpService.h"
namespace Sentry
{
    class ServerWeb : public PhysicalHttpService
    {
    public:
        ServerWeb() = default;
        ~ServerWeb() final = default;
    private:
        bool OnInit() final;
    private:
        int Info(Json::Writer & response);
		int Hotfix(Json::Writer& response);
        int Login(const Http::Request& request, Http::DataResponse& response);
        int Register(const Http::Request& request, Http::DataResponse& response);
    };
}


#endif //APP_SERVERWEB_H
