﻿#include "NetSessionManager.h"
#include "ScriptManager.h"
#include "ActionManager.h"
#include <Core/Applocation.h>
#include <Util/StringHelper.h>
#include <NetWork/TcpClientSession.h>
#include <Manager/ListenerManager.h>
#include <Manager/ActionManager.h>
#include <Manager/NetProxyManager.h>
#include <Manager/ProtocolManager.h>

namespace Sentry
{
    NetSessionManager::NetSessionManager()
    {
        this->mIsClose = false;
        this->mNetThread = nullptr;
    }

    bool NetSessionManager::OnInit()
    {
        this->mListenerManager = this->GetManager<ListenerManager>();
        SayNoAssertRetFalse_F(this->mNetProxyManager = this->GetManager<NetProxyManager>());
        SayNoAssertRetFalse_F(this->mLocalActionManager = this->GetManager<ActionManager>());
        return true;
    }

    void NetSessionManager::OnConnectSuccess(TcpClientSession *session)
    {
#ifdef SOEASY_DEBUG
        SayNoAssertRet_F(this->IsInNetThead());
#endif
        const std::string &address = session->GetAddress();
        const std::string &name = session->GetSessionName();
        Net2MainEvent *eve = new Net2MainEvent(SocketConnectSuc, address, name);
        if (eve != nullptr)
        {
            this->mRecvSessionQueue.push(address);
            this->mNetProxyManager->AddNetSessionEvent(eve);
        }
    }

    void NetSessionManager::OnSessionError(TcpClientSession *session, Net2MainEventType type)
    {
#ifdef SOEASY_DEBUG
        SayNoAssertRet_F(this->IsInNetThead());
#endif
        const std::string &address = session->GetAddress();
        const std::string &name = session->GetSessionName();
        Net2MainEvent *eve = new Net2MainEvent(type, address, name);
        this->mNetProxyManager->AddNetSessionEvent(eve);
    }


    bool NetSessionManager::OnRecvMessage(TcpClientSession *session, const char *message, const size_t size)
    {
        if (message == nullptr || size == 0)
        {
            return false;
        }

        NetMessageProxy *messageData = NetMessageProxy::Create(message, size);
        if (messageData == nullptr)
        {
            return false;
        }

        const std::string &name = session->GetSessionName();
        this->mRecvSessionQueue.push(session->GetAddress());
        Net2MainEvent *eve = new Net2MainEvent(SocketReceiveData, session->GetAddress(), name, messageData);
        return this->mNetProxyManager->AddNetSessionEvent(eve);
    }

    bool NetSessionManager::OnSendMessageError(TcpClientSession *session, const char *message, const size_t size)
    {
#ifdef SOEASY_DEBUG
        SayNoAssertRetFalse_F(this->IsInNetThead());
#endif
        if (message == nullptr || size == 0)
        {
            return false;
        }

        NetMessageProxy *messageData = NetMessageProxy::Create(message, size);
        if (messageData == nullptr)
        {
            return false;
        }

        const std::string &address = session->GetAddress();
        const std::string &name = session->GetSessionName();
        Net2MainEvent *eve = new Net2MainEvent(SocketSendMsgFail, address, name, messageData);
        return this->mNetProxyManager->AddNetSessionEvent(eve);
    }

    bool NetSessionManager::AddNetSessionEvent(Main2NetEvent *eve)
    {
#ifdef SOEASY_DEBUG
        SayNoAssertRetFalse_F(!this->IsInNetThead());
#endif
        if (eve->GetEventType() == SocketSendMsgEvent)
        {
            if (eve->GetMsgData() == nullptr)
            {
                return false;
            }
        }
        this->mNetEventQueue.AddItem(eve);
        return true;
    }

    TcpClientSession *NetSessionManager::Create(shared_ptr<AsioTcpSocket> socket)
    {
#ifdef SOEASY_DEBUG
        SayNoAssertRetNull_F(this->IsInNetThead());
#endif
        TcpClientSession *session = new TcpClientSession(this, socket);
        if (this->mSessionAdressMap.find(session->GetAddress()) == this->mSessionAdressMap.end())
        {
            this->mRecvSessionQueue.push(session->GetAddress());
            this->mSessionAdressMap.emplace(session->GetAddress(), session);
            Net2MainEvent *eve = new Net2MainEvent(SocketNewConnect, session->GetAddress());
            this->mNetProxyManager->AddNetSessionEvent(eve);
            return session;
        }
        return nullptr;
    }

    TcpClientSession *NetSessionManager::Create(const std::string &name, const std::string &address)
    {
#ifdef SOEASY_DEBUG
        SayNoAssertRetNull_F(this->IsInNetThead());
#endif
        auto iter = this->mSessionAdressMap.find(address);
        if (iter != this->mSessionAdressMap.end())
        {
            return iter->second;
        }
        std::string ip;
        unsigned short port;
        if (StringHelper::ParseIpAddress(address, ip, port))
        {
            TcpClientSession *session = new TcpClientSession(this, name, ip, port);
            this->mSessionAdressMap.emplace(address, session);
            return session;
        }
        return nullptr;
    }

    void NetSessionManager::OnDestory()
    {
    }

    void NetSessionManager::OnInitComplete()
    {
        this->mNetThread = new std::thread(BIND_THIS_ACTION_0(NetSessionManager::NetUpdate));
        this->mNetThread->detach();
    }

    void NetSessionManager::NetUpdate()
    {
        std::chrono::milliseconds time(1);
        Main2NetEvent *sessionEvent = nullptr;
        this->mNetThreadId = std::this_thread::get_id();
        while (this->mIsClose == false)
        {
            this_thread::sleep_for(time);

            this->mAsioContext.poll();
            if (this->mListenerManager)
            {
                this->mListenerManager->StartAccept();
            }

            while (!this->mRecvSessionQueue.empty())
            {
                TcpClientSession *tcpSession = this->GetSession(this->mRecvSessionQueue.front());
                if (tcpSession != nullptr && tcpSession->IsActive())
                {
                    tcpSession->StartReceiveMsg();
                }
                this->mRecvSessionQueue.pop();
            }

            this->mNetEventQueue.SwapQueueData(); //处理主线程过来的数据
            while (this->mNetEventQueue.PopItem(sessionEvent))
            {
                this->HandlerMainThreadEvent(sessionEvent);
                delete sessionEvent;
            }
        }
    }

    bool NetSessionManager::IsInNetThead()
    {
        return this->mNetThreadId == std::this_thread::get_id();
    }

    void NetSessionManager::HandlerMainThreadEvent(Main2NetEvent *eve) //处理主线程过来的事件
    {
        if (eve == nullptr)
            return;
        const std::string &address = eve->GetAddress();
        if (eve->GetEventType() == SocketDectoryEvent)
        {
            this->DescorySession(address);
        } else if (eve->GetEventType() == SocketConnectEvent)
        {
            const std::string &name = eve->GetName();
            TcpClientSession *session = this->Create(name, address);
            if (session == nullptr || !session->StartConnect())
            {
                Net2MainEvent *eve = new Net2MainEvent(SocketConnectFail, address, name);
                this->mNetProxyManager->AddNetSessionEvent(eve);
            }
        } else if (eve->GetEventType() == SocketSendMsgEvent)
        {
            TcpClientSession *session = this->GetSession(address);
            if (session != nullptr)
            {
                NetMessageProxy *messageData = eve->GetMsgData();
                size_t size = messageData->WriteToBuffer(this->mSendSharedBuffer, ASIO_TCP_SEND_MAX_COUNT);
                if (size == 0)
                {
                    const std::string &name = session->GetSessionName();
                    Net2MainEvent *eve = new Net2MainEvent(Net2MainEventType::SocketSendMsgFail, address, name,
                                                           messageData);
                    this->mNetProxyManager->AddNetSessionEvent(eve);
                    return;
                }
                session->SendPackage(std::make_shared<std::string>(this->mSendSharedBuffer, size));
            }
        }
    }

    TcpClientSession *NetSessionManager::GetSession(const std::string &address)
    {
#ifdef SOEASY_DEBUG
        SayNoAssertRetNull_F(this->IsInNetThead());
#endif
        auto iter = this->mSessionAdressMap.find(address);
        return iter != this->mSessionAdressMap.end() ? iter->second : nullptr;
    }

    bool NetSessionManager::DescorySession(const std::string &address)
    {
        auto iter = this->mSessionAdressMap.find(address);
        if (iter != this->mSessionAdressMap.end())
        {
            TcpClientSession *session = iter->second;
            if (session != nullptr && session->IsActive())
            {
                session->StartClose();
            }
            this->mSessionAdressMap.erase(iter);
            SayNoDebugError("remove tcp session : " << address);
            return true;
        }
        return false;
    }
}