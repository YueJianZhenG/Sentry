﻿
#include"InnerNetComponent.h"
#include"String/StringHelper.h"
#include"Tcp/SocketProxy.h"
#include"InnerNetMessageComponent.h"
#include"Config/ServiceConfig.h"
#include"File/FileHelper.h"
#include"Component/OuterNetComponent.h"
#include"Component/NetThreadComponent.h"
#include"google/protobuf/util/json_util.h"
namespace Sentry
{
	void InnerNetComponent::Awake()
	{
        this->mNetComponent = nullptr;
        this->mMessageComponent = nullptr;
    }
	bool InnerNetComponent::LateAwake()
	{
        std::string path;
        if(!this->GetConfig().GetPath("user", path))
        {
            return false;
        }
        rapidjson::Document document;
        if(!Helper::File::ReadJsonFile(path, document))
        {
            return false;
        }
        auto iter = document.MemberBegin();
        for(; iter != document.MemberEnd(); iter++)
        {
            const std::string user(iter->name.GetString());
            const std::string passwd(iter->value.GetString());
            this->mUserMaps.emplace(user, passwd);
        }

        this->mOuterComponent = this->GetComponent<OuterNetComponent>();
        LOG_CHECK_RET_FALSE(this->mNetComponent = this->GetComponent<NetThreadComponent>());
        LOG_CHECK_RET_FALSE(this->GetConfig().GetMember("user", "name", this->mUserName));
        LOG_CHECK_RET_FALSE(this->GetConfig().GetMember("user", "passwd", this->mUserName));
        LOG_CHECK_RET_FALSE(this->mMessageComponent = this->GetComponent<InnerNetMessageComponent>());
		return true;
	}

    void InnerNetComponent::OnMessage(const std::string &address, std::shared_ptr<Rpc::Data> message)
    {
        CONSOLE_LOG_ERROR("receive message typ " << message->GetType());
        switch ((Tcp::Type) message->GetType())
        {
            case Tcp::Type::Auth:
            {
                if (!this->OnAuth(address, message))
                {
                    this->StartClose(address);
                    CONSOLE_LOG_ERROR("auth error " << address);
                }
            }
                break;
            case Tcp::Type::Request:
            {
                if (!this->IsAuth(address))
                {
                    this->StartClose(address);
                    CONSOLE_LOG_ERROR("request message error close : " << address);
                }
                this->OnRequest(address, message);
            }
                break;
            case Tcp::Type::Forward:
            {
                if (!this->IsAuth(address))
                {
                    this->StartClose(address);
                    CONSOLE_LOG_ERROR("request message error close : " << address);
                }
                this->OnForward(message);
            }
                break;
            case Tcp::Type::Broadcast:
            {
                if (!this->IsAuth(address))
                {
                    this->StartClose(address);
                    CONSOLE_LOG_ERROR("request message error close : " << address);
                }
                this->OnBroadcast(message);
            }
                break;
            case Tcp::Type::Response:
            {
                if (!this->IsAuth(address))
                {
                    this->StartClose(address);
                    CONSOLE_LOG_ERROR("request message error close : " << address);
                }
                this->OnResponse(address, message);
            }
                break;
            default:
            {
                std::string func;
                message->GetHead().Get("func", func);
                LOG_ERROR("call " << func << " type error");
            }
        }
    }

    bool InnerNetComponent::OnAuth(const std::string & address, std::shared_ptr<Rpc::Data> message)
    {
        InnerServerNode serverNode;
        const Rpc::Head &head = message->GetHead();
        LOG_CHECK_RET_FALSE(head.Get("user", serverNode.UserName));
        LOG_CHECK_RET_FALSE(head.Get("passwd", serverNode.PassWord));
        LOG_CHECK_RET_FALSE(head.Get("location", serverNode.Location));
        auto iter = this->mUserMaps.find(serverNode.UserName);
        if (iter == this->mUserMaps.end() || iter->second != serverNode.PassWord)
        {
            CONSOLE_LOG_ERROR(address << " auth failure");
            return false;
        }
        this->mLocationMaps.emplace(address, serverNode);
        return true;
    }

    bool InnerNetComponent::IsAuth(const std::string &address)
    {
        auto iter = this->mRpcClientMap.find(address);
        return iter != this->mRpcClientMap.end();
    }

	bool InnerNetComponent::OnRequest(const std::string& address, std::shared_ptr<Rpc::Data> message)
	{
        message->GetHead().Add("address", address);
        LOG_CHECK_RET_FALSE(message->GetHead().Has("func"));
        XCode code = this->mMessageComponent->OnRequest(message);
        if(code != XCode::Successful)
        {
            std::string func;
            message->GetHead().Get("func", func);
            CONSOLE_LOG_ERROR("call " << func << " code = " << (int)code);
            return false;
        }
        return true;
	}

    bool InnerNetComponent::OnForward(std::shared_ptr<Rpc::Data> message)
    {
        long long userId = 0;
        LOG_CHECK_RET_FALSE(this->mOuterComponent != nullptr);
        LOG_CHECK_RET_FALSE(message->GetHead().Has("func"));
        LOG_CHECK_RET_FALSE(message->GetHead().Get("id", userId));
        message->GetHead().Remove("id");
        message->SetType(Tcp::Type::Request);
        message->GetHead().Remove("address");
        return this->mOuterComponent->SendData(userId, message);
    }

    bool InnerNetComponent::OnBroadcast(std::shared_ptr<Rpc::Data> message)
    {
        LOG_CHECK_RET_FALSE(this->mOuterComponent != nullptr);
        LOG_CHECK_RET_FALSE(message->GetHead().Has("func"));
        message->SetType(Tcp::Type::Request);
        message->GetHead().Remove("address");
        return this->mOuterComponent->SendData(message);
    }

	bool InnerNetComponent::OnResponse(const std::string& address, std::shared_ptr<Rpc::Data> message)
	{
        const Rpc::Head &head = message->GetHead();
        if(this->mOuterComponent != nullptr)
        {
            std::string address;
            if (head.Get("client", address)) //address
            {
                message->GetHead().Remove("client");
                this->mOuterComponent->SendData(address, message);
                return true;
            }
        }
#ifdef __DEBUG__
        int code = 0;
        LOG_CHECK_RET_FALSE(head.Get("code", code));
        if(code != (int)XCode::Successful)
        {
            std::string error, func;
            if(head.Get("func", func) && head.Get("error", error))
            {
                CONSOLE_LOG_ERROR("func = " << func << "  error = " << error);
            }
        }
#endif
        long long rpcId = 0;
        LOG_CHECK_RET_FALSE(head.Get("rpc", rpcId));
		this->mMessageComponent->OnResponse(rpcId, message);
		return true;
	}

	void InnerNetComponent::OnCloseSocket(const std::string & address, XCode code)
	{
		auto iter = this->mRpcClientMap.find(address);
		if (iter != this->mRpcClientMap.end())
		{
			this->mRpcClientMap.erase(iter);
			LOG_WARN("close server address : " << address);
		}
	}

	bool InnerNetComponent::OnListen(std::shared_ptr<SocketProxy> socket)
	{
		const std::string& address = socket->GetAddress();
		auto iter = this->mRpcClientMap.find(address);
		if (iter == this->mRpcClientMap.end())
		{
			assert(!address.empty());
			std::shared_ptr<InnerNetClient> tcpSession
					= std::make_shared<InnerNetClient>(this, socket);

			tcpSession->StartReceive();
			this->mRpcClientMap.emplace(address, tcpSession);
            return true;
		}
        return false;
	}

	void InnerNetComponent::StartClose(const std::string & address)
	{
		auto iter = this->mRpcClientMap.find(address);
		if (iter != this->mRpcClientMap.end())
		{
			std::shared_ptr<InnerNetClient> innerNetClient = iter->second;
            if(innerNetClient != nullptr)
            {
                innerNetClient->StartClose();
            }
			this->mRpcClientMap.erase(iter);
		}
	}

	std::shared_ptr<InnerNetClient> InnerNetComponent::GetOrCreateSession(const std::string& address)
	{
		std::shared_ptr<InnerNetClient> localSession = this->GetSession(address);
		if (localSession != nullptr)
		{
			return localSession;
		}
		std::string ip;
		unsigned short port = 0;
        if(!Helper::String::ParseIpAddress(address, ip, port))
        {
            CONSOLE_LOG_ERROR("parse address error : [" << address << "]");
            return nullptr;
        }
		std::shared_ptr<SocketProxy> socketProxy = this->mNetComponent->CreateSocket();
        if(socketProxy == nullptr)
        {
            return nullptr;
        }
        socketProxy->Init(ip, port);
		localSession = std::make_shared<InnerNetClient>(this, socketProxy);

		this->mRpcClientMap.emplace(socketProxy->GetAddress(), localSession);
		return localSession;
	}

	std::shared_ptr<InnerNetClient> InnerNetComponent::GetSession(const std::string& address)
	{
		auto iter = this->mRpcClientMap.find(address);
		if (iter == this->mRpcClientMap.end())
		{
			return nullptr;
		}
		return iter->second;
	}


	bool InnerNetComponent::Send(const std::string & address, std::shared_ptr<Rpc::Data> message)
	{
        assert(message->GetType() == (int)Tcp::Type::Request
            || message->GetType() == (int)Tcp::Type::Response);
        std::shared_ptr<InnerNetClient> clientSession = this->GetOrCreateSession(address);
		if (message == nullptr || clientSession == nullptr)
		{
			return false;
		}
		clientSession->SendData(message);
		return true;
	}
}// namespace Sentry