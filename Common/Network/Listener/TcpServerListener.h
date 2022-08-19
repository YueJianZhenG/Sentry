#pragma once
#include"Async/TaskSource.h"
#include"Global/ServerConfig.h"
#include"Define/CommonTypeDef.h"

namespace Sentry
{

	class ISocketListen;
    class TcpServerComponent;
    class NetThreadComponent;

    class TcpServerListener
	{
	 public:
		TcpServerListener();
		~TcpServerListener();
	 public:
        bool StartListen();
        bool Init(const ListenConfig * config);
        virtual bool StartInComplete() { return true; }
        const ListenConfig & GetListenConfig() const { return *this->mConfig; }
        const std::string & GetListenAddress() const { return this->mConfig->Address; }
    protected:
        void ListenConnect();
        virtual bool OnListen(std::shared_ptr<SocketProxy> socket) = 0;
    private:
		int mCount;
		int mErrorCount;
		const ListenConfig * mConfig;
        AsioTcpAcceptor* mBindAcceptor;
        TcpServerComponent * mTcpComponent;
        NetThreadComponent * mNetComponent;
    };
}