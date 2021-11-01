﻿#pragma once
#include<Thread/TaskThread.h>
#include<Network/SessionBase.h>
#include<Protocol/com.pb.h>
#include<Pool/StringPool.h>
namespace GameKeeper
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
        virtual bool OnRequestMessage(const com::DataPacket_Request & message) = 0;
    };

    class IResponseMessageHandler
    {
    public:
        virtual bool OnResponseMessage(const com::DataPacket_Response & message) = 0;
    };
}

namespace GameKeeper
{
	class NetWorkThread;
    class TcpClientSession;
	
    class ISocketHandler
    {
    public:
		ISocketHandler() { this->mNetThread = nullptr; }
		virtual ~ISocketHandler() { }
	public:
        virtual SessionBase * CreateSocket() = 0;
        virtual void OnClose(SessionBase * socket) = 0;
        virtual void OnSessionErr(SessionBase * session, const asio::error_code & err) = 0;
        virtual void OnListenConnect(SessionBase * session, const asio::error_code & err) = 0;
        virtual void OnConnectRemote(SessionBase * session, const asio::error_code & err) = 0;
		virtual void OnReceiveNewMessage(SessionBase * session, string * message) = 0;
		virtual void OnSendMessage(SessionBase * session, string * message, const asio::error_code & err) = 0;
	public:
        NetWorkThread * GetNetThread()
        {
            return mNetThread;
        };
		void SetNetThread(NetWorkThread * t)
        {
            this->mNetThread = t;
			this->mNetThread->Start();
        };
        StringPool & GetStringPool() { return this->mStringPool;}
        AsioContext & GetContext() { return mNetThread->GetContext();}

    protected:
        StringPool mStringPool;
        NetWorkThread * mNetThread;
    };

    template<typename T>
    class SocketHandler : public ISocketHandler
    {
    public:
        void OnClose(SessionBase * socket) final
        {
            this->OnCloseSession(static_cast<T*>(socket));
        };
        void OnSessionErr(SessionBase * session, const asio::error_code & err) final
        {
            this->OnSessionError(static_cast<T*>(session), err);
        }
        void OnConnectRemote(SessionBase * session, const asio::error_code & err) final
        {
            this->OnConnectRemoteAfter(static_cast<T*>(session), err);
        }
        void OnListenConnect(SessionBase * session, const asio::error_code & err) final
        {
            this->OnListenNewSession(static_cast<T*>(session), err);
        }
        void OnReceiveNewMessage(SessionBase * session, string * message) final
        {
            if(!this->OnReceiveMessage(static_cast<T*>(session), *message))
            {
                session->Close();
            }
            this->mStringPool.Destory(message);
        }

        void OnSendMessage(SessionBase *session, string * message, const asio::error_code &err) final
        {
            this->OnSendMessageAfter(static_cast<T*>(session), *message, err);
            this->mStringPool.Destory(message);
        }
    protected:
        virtual void OnCloseSession(T * session) { };
		virtual bool OnReceiveMessage(T * session, const string & message) { return false; };
		virtual void OnSessionError(T * session, const asio::error_code & err) { };
        virtual void OnListenNewSession(T * session, const asio::error_code & err) { };
		virtual void OnConnectRemoteAfter(T * session, const asio::error_code & err) { };
        virtual void OnSendMessageAfter(T * session, const std::string & message, const asio::error_code & err) { };
    };
}