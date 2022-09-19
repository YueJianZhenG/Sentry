//
// Created by mac on 2022/4/14.
//
#ifdef __ENABLE_MYSQL__
#ifndef SERVER_MYSQLCLIENT_H
#define SERVER_MYSQLCLIENT_H

#include"MysqlDefine.h"
#include"MysqlMessage.h"
#include"Network/TcpContext.h"
namespace Sentry
{
    class MysqlDBComponent;

    class MysqlClient
    {
    public:
        MysqlClient(const MysqlConfig &config, MysqlDBComponent *component);

    public:
        void Stop();
        bool Start();
        bool StartConnect();
        size_t GetTaskCount() const { return this->mTaskCount; }
        void SendCommand(std::shared_ptr<Mysql::ICommand> command);

    private:
        void Update();
        bool GetCommand(std::shared_ptr<Mysql::ICommand> &command);
    private:
        bool mIsClose;
        std::mutex mLock;
        size_t mTaskCount;
        MYSQL *mMysqlClient;
        std::thread *mThread;
        long long mLastTime;
        const MysqlConfig &mConfig;
        MysqlDBComponent *mComponent;
        std::queue<std::shared_ptr<Mysql::ICommand>> mCommands;
    };
}
#endif //SERVER_MYSQLCLIENT_H
#endif
