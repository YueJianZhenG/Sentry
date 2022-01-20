//
// Created by zmhy0073 on 2021/10/26.
//
#include<string>
#ifndef GameKeeper_SERVERPATH_H
#define GameKeeper_SERVERPATH_H

namespace Sentry
{
    class ServerPath
    {
    public:
        ServerPath(int argc, char ** argv);
    public:
        const std::string & GetLogPath() const { return this->mLogsPath;}
        const std::string & GetWorkPath() const { return this->mWorkPath;}
        const std::string & GetConfigPath() const { return this->mConfigPath;}
		const std::string & GetDownloadPath() const { return this->mDownloadPath; }
    private:
        std::string mLogsPath;
        std::string mWorkPath;
        std::string mConfigPath;
		std::string mDownloadPath;
    };
}

#endif //GameKeeper_SERVERPATH_H
