﻿
#include"InnerNetComponent.h"
#include"XCode/XCode.h"
#include"Entity/Unit/App.h"
#include"Util/String/StringHelper.h"
#include"Network/Tcp/SocketProxy.h"
#include"DispatchComponent.h"
#include"Util/File/FileHelper.h"
#include"Server/Component/ThreadComponent.h"
#include"google/protobuf/util/json_util.h"
#include"Core/Singleton/Singleton.h"

namespace Tendo
{
    InnerNetComponent::InnerNetComponent()
    {
        this->mSumCount = 0;
        this->mMaxHandlerCount = 0;
        this->mNetComponent = nullptr;
        this->mMessageComponent = nullptr;
    }

    bool InnerNetComponent::LateAwake()
    {
        std::string path;
        rapidjson::Document document;
        this->mMaxHandlerCount = 2000;
        const ServerConfig *config = ServerConfig::Inst();
        LOG_CHECK_RET_FALSE(config->GetPath("user", path));
        config->GetMember("message", "inner", this->mMaxHandlerCount);
        LOG_CHECK_RET_FALSE(config->GetLocation("rpc", this->mLocation));
        LOG_CHECK_RET_FALSE(Helper::File::ReadJsonFile(path, document));
        auto iter = document.MemberBegin();
        for (; iter != document.MemberEnd(); iter++)
        {
            const std::string user(iter->name.GetString());
            const std::string passwd(iter->value.GetString());
            this->mUserMaps.emplace(user, passwd);
        }
        NodeInfo nodeInfo;
        nodeInfo.SrvName = ServerConfig::Inst()->Name();
        config->GetLocation("rpc", nodeInfo.RpcAddress);
        config->GetLocation("http", nodeInfo.HttpAddress);
        this->mLocationMaps.emplace(nodeInfo.RpcAddress, nodeInfo);
        LOG_CHECK_RET_FALSE(this->mNetComponent = this->GetComponent<ThreadComponent>());
        LOG_CHECK_RET_FALSE(this->mMessageComponent = this->GetComponent<DispatchComponent>());
        return this->StartListen("rpc");
    }

    void InnerNetComponent::OnConnectSuccessful(const std::string &address)
    {

    }

    void InnerNetComponent::OnMessage(std::shared_ptr<Msg::Packet> message)
    {
        this->mSumCount++;
        int type = message->GetType();
		message->SetNet(Msg::Net::Tcp);
		const std::string &address = message->From();
        if (type != Msg::Type::Auth && address != this->mLocation)
        {
            if (!this->IsAuth(address))
            {
                this->StartClose(address);
                CONSOLE_LOG_ERROR("close " << address << " not auth");
                return;
            }
        }
        switch (type)
        {
            case Msg::Type::Auth:
                this->OnAuth(message);
                break;
            case Msg::Type::Logout:
                this->StartClose(address);
                break;
            case Msg::Type::Request:
                this->mWaitMessages.push(message);
                break;
            case Msg::Type::Forward:
            case Msg::Type::Response:
            case Msg::Type::Broadcast:
                this->mMessageComponent->OnMessage(message);
                break;
            default:
            LOG_FATAL("unknown message type : " << message->GetType());
                break;
        }
    }

    void InnerNetComponent::OnSendFailure(const std::string &address, std::shared_ptr<Msg::Packet> message)
    {
        if (message->GetType() == Msg::Type::Request)
        {
            if (message->GetHead().Has("rpc"))
            {
                message->SetType(Msg::Type::Response);
                message->GetHead().Add("code", XCode::NetWorkError);
                this->mMessageComponent->OnMessage(message);
                return;
            }
        }
    }

    bool InnerNetComponent::OnAuth(const std::shared_ptr<Msg::Packet> &message)
    {
        NodeInfo nodeInfo;
        const Msg::Head &head = message->GetHead();
        const std::string &address = message->From();

        LOG_CHECK_RET_FALSE(head.Get("name", nodeInfo.SrvName));
        LOG_CHECK_RET_FALSE(head.Get("user", nodeInfo.UserName));
        LOG_CHECK_RET_FALSE(head.Get("rpc", nodeInfo.RpcAddress));
        LOG_CHECK_RET_FALSE(head.Get("passwd", nodeInfo.PassWord));
        if (nodeInfo.RpcAddress.empty())
        {
            this->StartClose(address);
            return false;
        }
        if (!this->mUserMaps.empty())
        {
            auto iter = this->mUserMaps.find(nodeInfo.UserName);
            if (iter == this->mUserMaps.end() || iter->second != nodeInfo.PassWord)
            {
                this->StartClose(address);
                CONSOLE_LOG_ERROR(address << " auth failure");
                return false;
            }
        }
        nodeInfo.LocalAddress = address;
        this->mLocationMaps.emplace(address, nodeInfo);
        return true;
    }

    bool InnerNetComponent::IsAuth(const std::string &address)
    {
        auto iter = this->mRemoteClients.find(address);
        return iter != this->mRemoteClients.end();
    }


    void InnerNetComponent::OnCloseSocket(const std::string &address, int code)
    {
        auto iter = this->mRemoteClients.find(address);
        if (iter != this->mRemoteClients.end())
        {
            this->mRemoteClients.erase(iter);
            LOG_WARN("close server address : " << address);
        }
    }

    void InnerNetComponent::OnListen(std::shared_ptr<Tcp::SocketProxy> socket)
    {
        const std::string &address = socket->GetAddress();
        auto iter = this->mRemoteClients.find(address);
        if (iter == this->mRemoteClients.end())
        {
            assert(!address.empty());
            std::shared_ptr<InnerNetTcpClient> tcpSession
                    = std::make_shared<InnerNetTcpClient>(this, socket);

            tcpSession->StartReceive();
            this->mRemoteClients.emplace(address, tcpSession);
        }
    }

    void InnerNetComponent::StartClose(const std::string &address)
    {
        auto iter = this->mRemoteClients.find(address);
        if (iter != this->mRemoteClients.end())
        {
            std::shared_ptr<InnerNetTcpClient> innerNetClient = iter->second;
            if (innerNetClient != nullptr)
            {
                innerNetClient->StartClose();
            }
            this->mRemoteClients.erase(iter);
        }
    }

    InnerNetTcpClient *InnerNetComponent::GetLocalClient(const std::string& address)
    {
		auto iter = this->mRemoteClients.find(address);
		if(iter != this->mRemoteClients.end())
		{
			return iter->second.get();
		}
		auto iter1 = this->mLocalClients.find(address);
		if(iter1 != this->mLocalClients.end())
		{
			return iter1->second.get();
		}
        std::string ip;
		std::string net;
        unsigned short port = 0;
        if (!Helper::Str::SplitAddr(address, net, ip, port))
        {
            CONSOLE_LOG_ERROR("parse address error : [" << address << "]");
            return nullptr;
        }
		AuthInfo authInfo;
        const ServerConfig *config = ServerConfig::Inst();
        {
            authInfo.ServerName = config->Name();
            config->GetLocation("rpc", authInfo.RpcAddress);
            config->GetMember("user", "name", authInfo.UserName);
            config->GetMember("user", "passwd", authInfo.PassWord);
        }
		std::shared_ptr<Tcp::SocketProxy> socketProxy = this->mNetComponent->CreateSocket(net, ip, port);
        std::shared_ptr<InnerNetTcpClient> localClient = std::make_shared<InnerNetTcpClient>(this, socketProxy, authInfo);

        this->mLocalClients.emplace(socketProxy->GetAddress(), localClient);
        return localClient.get();
    }

    InnerNetTcpClient *InnerNetComponent::GetRemoteClient(const std::string& address)
    {
        auto iter = this->mRemoteClients.find(address);
        if (iter == this->mRemoteClients.end())
        {
            return nullptr;
        }
        return iter->second.get();
    }


    bool InnerNetComponent::Send(const std::shared_ptr<Msg::Packet> &message)
    {
		message->SetNet(Msg::Net::Tcp);
        message->SetFrom(this->mLocation);
        this->mMessageComponent->OnMessage(message);
        return true;
    }

    bool InnerNetComponent::Send(const std::string &address, int code, const std::shared_ptr<Msg::Packet> &message)
    {
        if (!message->GetHead().Has("rpc"))
        {
            return false; //不需要返回
        }
        message->GetHead().Remove("id");
        message->GetHead().Add("code", code);
#ifndef __DEBUG__
        message->GetHead().Remove("func");
#endif
        message->SetType(Msg::Type::Response);
        this->Send(message->From(), message);
        return true;
    }

    bool InnerNetComponent::Send(const std::string &address, const std::shared_ptr<Msg::Packet> &message)
    {
        if (address == this->mLocation) //发送到本机
        {
            message->SetFrom(address);
            //this->mMessageComponent->OnMessage(message);
            Asio::Context &io = this->mApp->MainThread();
            io.post(std::bind(&InnerNetComponent::OnMessage, this, message));
            return true;
        }
        InnerNetTcpClient *clientSession = nullptr;
        switch (message->GetType())
        {
            case Msg::Type::Response:
                clientSession = this->GetRemoteClient(address);
                break;
            default:
                clientSession = this->GetLocalClient(address);
                break;
        }
        if (clientSession == nullptr)
        {
            LOG_ERROR("not find rpc client : [" << address << "]");
            return false;
        }
        clientSession->Send(message);
        return true;
    }

    std::shared_ptr<Msg::Packet> InnerNetComponent::Call(const std::string & address, const std::shared_ptr<Msg::Packet> & message)
    {
        int taskId = this->mMessageComponent->PopTaskId();
        message->GetHead().Add("rpc", taskId);
        if(!this->Send(address, message))
        {
            this->mMessageComponent->PushTaskId(taskId);
            return nullptr;
        }
        std::shared_ptr<RpcTaskSource> taskSource =
                std::make_shared<RpcTaskSource>(taskId);
       return  this->mMessageComponent->AddTask(taskId, taskSource)->Await();
    }

    bool InnerNetComponent::Send(const std::string & address, const std::shared_ptr<Msg::Packet>& message, int & taskId)
    {
        taskId = this->mMessageComponent->PopTaskId();
        message->GetHead().Add("rpc", taskId);
        if(!this->Send(address, message))
        {
            this->mMessageComponent->PushTaskId(taskId);
            return false;
        }
        return true;
    }

    void InnerNetComponent::OnRecord(Json::Writer &document)
    {
        document.Add("sum").Add(this->mSumCount);
        document.Add("wait").Add(this->mMessageComponent->WaitCount());
        document.Add("auth").Add(this->mLocationMaps.size());
        document.Add("client").Add(this->mRemoteClients.size());
    }

    const NodeInfo *InnerNetComponent::GetNodeInfo(const std::string &address) const
    {
        auto iter = this->mLocationMaps.find(address);
        return iter != this->mLocationMaps.end() ? &iter->second : nullptr;
    }

    void InnerNetComponent::OnFrameUpdate(float t)
    {
        for (int index = 0; index < this->mMaxHandlerCount && !this->mWaitMessages.empty(); index++)
        {
            std::shared_ptr<Msg::Packet> message = this->mWaitMessages.front();
            {
                int code = this->mMessageComponent->OnMessage(message);
                if (code != XCode::Successful && message->GetHead().Has("rpc"))
                {
                    message->Clear();
                    message->GetHead().Add("code", code);
                    message->SetType(Msg::Type::Response);
                    this->Send(message->From(), message);
                }
            }
            this->mWaitMessages.pop();
        }
    }

    size_t InnerNetComponent::GetConnectClients(std::vector<std::string> &list) const
    {
        size_t count = 0;
		list.reserve(this->mRemoteClients.size());
        auto iter = this->mRemoteClients.begin();
        for (; iter != this->mRemoteClients.end(); iter++)
		{
			count++;
			list.emplace_back(iter->first);
		}
        return count;
    }


    size_t InnerNetComponent::Broadcast(const std::shared_ptr<Msg::Packet> &message) const
    {
        size_t count = 0;
        auto iter = this->mRemoteClients.begin();
        for (; iter != this->mRemoteClients.end(); iter++)
		{
			count++;
			iter->second->Send(message->Clone());
		}
        return count;
    }

    void InnerNetComponent::OnDestroy()
    {
        this->StopListen();
        auto iter = this->mRemoteClients.begin();
        for (; iter != this->mRemoteClients.end(); iter++)
        {
            this->StartClose(iter->first);
        }
    }

}// namespace Sentry