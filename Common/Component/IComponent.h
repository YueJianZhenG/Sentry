﻿#pragma once
#include<Thread/TaskThread.h>
#include<NetWork/SessionBase.h>
namespace Sentry
{
	class IFrameUpdate
	{
	public:
		virtual void OnFrameUpdate(float t) = 0;
	};

	class ISystemUpdate
	{
	public:
		virtual void OnSystemUpdate() = 0;
	};

	class ISecondUpdate
	{
	public:
		virtual void OnSecondUpdate() = 0;
	};

	class ILastFrameUpdate
	{
	public:
		virtual void OnLastFrameUpdate() = 0;
	};

	class IHotfix
	{
	public:
		virtual void OnHotFix() = 0;
	};

    class ILoadData
    {
    public:
        virtual void OnLodaData() = 0;
    };

    class IZeroRefresh
    {
    public:
        virtual void OnZeroRefresh() = 0;
    };

    class IRequestMessageHandler
    {
    public:
        virtual bool OnRequestMessage(const std::string & address, SharedMessage message) = 0;
    };

    class IResponseMessageHandler
    {
    public:
        virtual bool OnResponseMessage(const std::string & address, SharedMessage message) = 0;
    };
}

namespace Sentry
{
	class NetWorkThread;
    class TcpClientSession;
	
    class ISocketHandler
    {
    public:

        virtual SessionBase * CreateSocket() = 0;
        virtual void OnClose(SessionBase * socket) = 0;
        virtual void OnListenConnect(SessionBase * session) = 0;
        virtual void OnSessionErr(SessionBase * session, const asio::error_code & err) = 0;
		virtual void OnConnectRemote(SessionBase * session, const asio::error_code & err) = 0;
		virtual void OnReceiveNewMessage(SessionBase * session, SharedMessage message) = 0;
		virtual void OnSendMessage(SessionBase * session, SharedMessage message, const asio::error_code & err) = 0;
	public:
        NetWorkThread * GetNetThread()
        {
            return mNetThread;
        };
		void SetNetThread(NetWorkThread * t)
        {
            this->mNetThread = t;
        };
        AsioContext & GetContext() { return mNetThread->GetContext();}
	private:
		NetWorkThread * mNetThread;
    };

    template<typename T>
    class ScoketHandler : public ISocketHandler
    {
    public:
        void OnClose(SessionBase * socket) override
        {
            this->OnCloseSession(static_cast<T*>(socket));
        };
        void OnSessionErr(SessionBase * session, const asio::error_code & err) override
        {
            this->OnSessionError(static_cast<T*>(session), err);
        }
        void OnConnectRemote(SessionBase * session, const asio::error_code & err) override
        {
            this->OnConnectRemoteAfter(static_cast<T*>(session), err);
        }
        void OnListenConnect(SessionBase * session) override
        {
            if(!this->OnListenNewSession(static_cast<T*>(session)))
            {
                session->Close();
            }
        }
        void OnReceiveNewMessage(SessionBase * session, SharedMessage message) override
        {
            if(!this->OnReceiveMessage(static_cast<T*>(session), message))
            {
                session->Close();
            }
        }

        void OnSendMessage(SessionBase *session, SharedMessage message, const asio::error_code &err) override
        {
            this->OnSendMessageAfter(static_cast<T*>(session), message, err);
        }
    protected:
        virtual void OnCloseSession(T * session) = 0;
        virtual bool OnListenNewSession(T * session) = 0;
        virtual bool OnReceiveMessage(T * session, SharedMessage message) = 0;
        virtual void OnSessionError(T * session, const asio::error_code & err) = 0;
        virtual void OnConnectRemoteAfter(T * session, const asio::error_code & err) = 0;
        virtual void OnSendMessageAfter(T * session, SharedMessage message, const asio::error_code & err) = 0;
    };
}