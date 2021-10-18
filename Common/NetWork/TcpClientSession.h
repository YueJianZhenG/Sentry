﻿#pragma once

#include <Define/CommonDef.h>
#include <NetWork/SessionBase.h>

namespace Sentry
{

    enum ETcpErrorType
    {
        ErrNone,
        ErrConnect,
        ErrRead,
        ErrWrite,
    };
}


namespace Sentry
{
    class ISocketHandler;

    class TcpClientSession : public SessionBase
    {
    public:
        TcpClientSession(ISocketHandler *handler);
     
        virtual ~TcpClientSession();
	protected:
		void OnStartReceive();
    private:
        void ReadMessageBody(const size_t allSize);
    private:
        char *mRecvMsgBuffer;
        unsigned int mRecvBufferSize;
    };

    typedef shared_ptr<TcpClientSession> SharedTcpSession;

}// namespace Sentry