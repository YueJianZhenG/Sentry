#pragma once

#include <Protocol/s2s.pb.h>
#include <Service/LocalService.h>

namespace Sentry
{
    class MysqlProxy : public LocalService
    {
    public:
        MysqlProxy();

        ~MysqlProxy() {}

    public:
        XCode Add(const s2s::MysqlOper_Request &request, s2s::MysqlOper_Response &response);

        XCode Save(const s2s::MysqlOper_Request &request, s2s::MysqlOper_Response &response);

        XCode Delete(const s2s::MysqlOper_Request &request, s2s::MysqlOper_Response &response);

        XCode QueryData(const s2s::MysqlQuery_Request &request, s2s::MysqlQuery_Response &response);

    public:
        bool Awake() final;

        void Start() final;

    private:

        class SceneMysqlComponent *mMysqlManager;

        class CoroutineComponent *mCorComponent;

        class SceneTaskComponent *mTaskManager;

        class SceneProtocolComponent * mProtocolManager;
    };
}// namespace Sentry