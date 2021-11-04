﻿#include "TcpClientComponent.h"

#include <Core/App.h>
#include <Scene/CallHandlerComponent.h>
#include <Util/StringHelper.h>
#include <Scene/ProtocolComponent.h>
#include <Scene/TaskPoolComponent.h>
#include <Service/ServiceMgrComponent.h>
#include <Network/Tcp/TcpLocalSession.h>
#ifdef __DEBUG__
#include <Pool/MessagePool.h>
#endif
namespace GameKeeper
{
    bool TcpClientComponent::Awake()
    {
		GKAssertRetFalse_F(this->mTaskComponent = this->GetComponent<TaskPoolComponent>());
		GKAssertRetFalse_F(this->mProtocolComponent = this->GetComponent<ProtocolComponent>());
		GKAssertRetFalse_F(this->mServiceComponent = this->GetComponent<ServiceMgrComponent>());
        GKAssertRetFalse_F(this->mCallHandlerComponent = this->GetComponent<CallHandlerComponent>());

        return true;
    }

	void TcpClientComponent::OnCloseSession(TcpClientSession * socket)
	{
		if (socket == nullptr)
		{
			return;
		}
		const std::string & address = socket->GetAddress();
		long long id = socket->GetSocketProxy().GetSocketId();
		auto iter = this->mSessionAdressMap.find(id);
		if (iter != this->mSessionAdressMap.end())
		{
			this->mSessionAdressMap.erase(iter);
			GKDebugError("remove tcp socket " << address);
			delete socket;
		}
	}

	void TcpClientComponent::OnReceiveMessage(TcpClientSession * session, string * message)
	{
		if (!this->OnReceive(session, *message))
		{
			session->StartClose();
		}
		GStringPool.Destory(message);
	}

    void TcpClientComponent::OnSendMessageAfter(TcpClientSession *session, std::string * message, bool)
    {
#ifdef __DEBUG__
        const std::string & address = session->GetAddress();
        auto type = (DataMessageType)message->at(sizeof(unsigned int));
        if(type == DataMessageType::TYPE_REQUEST)
        {
            GKDebugWarning(address << " send request message successful size = " << message->size());
        }
        else if(type == DataMessageType::TYPE_RESPONSE)
        {
            GKDebugWarning(address << " send response message successful size = " << message->size());
        }
#endif
		GStringPool.Destory(message);
    }

	void TcpClientComponent::OnConnectRemoteAfter(TcpLocalSession *session, const asio::error_code &err)
	{
		const std::string & address = session->GetAddress();
		if (err)
		{
			GKDebugError("connect to " << address << " failure : " << err.message());
		}
		else
		{
			long long id = session->GetSocketProxy().GetSocketId();
			const std::string & name = session->GetSocketProxy().GetName();
			GKDebugInfo(
				"connect to [" << name << ":" << address << "] successful");
			this->mSessionAdressMap.emplace(id, session);
		}
	}

	void TcpClientComponent::OnListen(SocketProxy * socket)
	{
		long long id = socket->GetSocketId();
		auto iter = this->mSessionAdressMap.find(id);
		if (iter == this->mSessionAdressMap.end())
		{
			TcpClientSession * tcpSession = new TcpClientSession(this);
			tcpSession->SetSocket(socket);
			this->mSessionAdressMap.emplace(id, tcpSession);
		}
	}


	bool TcpClientComponent::OnReceive(TcpClientSession *session, const std::string & message)
	{
        const char * body = message.c_str() + 1;
        const size_t bodySize = message.size() - 1;
        const std::string & address = session->GetAddress();
		auto messageType = (DataMessageType)message.at(0);
		if (messageType == DataMessageType::TYPE_REQUEST)
        {
            this->mRequestData.Clear();
            if (!this->mRequestData.ParseFromArray(body, bodySize))
            {
                return false;
            }
            unsigned short methodId = this->mRequestData.methodid();
            auto config = this->mProtocolComponent->GetProtocolConfig(methodId);
            if (config == nullptr)
            {
                return false;
            }
#ifdef __DEBUG__
            std::string json;
            const std::string & data = this->mRequestData.messagedata();
            const std::string method = config->ServiceName + "." + config->Method;
            if(!config->RequestMessage.empty())
            {
               Message * msg = MessagePool::NewByData(config->RequestMessage, data);
               util::MessageToJsonString(*msg, &json);
            }
            GKDebugLog("[request " << method << "] json = " << json);
#endif
            this->mRequestData.set_address(address);
            return this->mServiceComponent->OnRequestMessage(mRequestData);
        }
		else if (messageType == DataMessageType::TYPE_RESPONSE)
		{
            this->mResponseData.Clear();
            if(!this->mResponseData.ParseFromArray(body, bodySize))
            {
                return false;
            }
            unsigned short methodId = this->mResponseData.methodid();
            auto config = this->mProtocolComponent->GetProtocolConfig(methodId);
            if (config == nullptr)
            {
                return false;
            }
#ifdef __DEBUG__
            std::string json;
            const std::string & data = this->mRequestData.messagedata();
            const std::string method = config->ServiceName + "." + config->Method;
            if(!config->ResponseMessage.empty())
            {
                Message * msg = MessagePool::NewByData(config->ResponseMessage, data);
                util::MessageToJsonString(*msg, &json);
            }
            GKDebugLog("[response " << method << "] code:" << this->mResponseData.code() << "  json = " << json);
#endif
            return this->mCallHandlerComponent->OnResponseMessage(mResponseData);
		}
        return false;
	}

    TcpLocalSession *TcpClientComponent::GetLocalSession(long long id)
    {
        auto iter = this->mSessionAdressMap.find(id);
        if(iter == this->mSessionAdressMap.end())
        {
            return nullptr;
        }
        TcpClientSession * session = iter->second;
        if(session->GetSocketType() == SocketType::RemoteSocket)
        {
            return nullptr;
        }
        return static_cast<TcpLocalSession*>(session);
    }

    TcpClientSession *TcpClientComponent::GetRemoteSession(long long id)
    {
        auto iter = this->mSessionAdressMap.find(id);
        return iter == this->mSessionAdressMap.end() ? nullptr : iter->second;
    }

    long long TcpClientComponent::NewSession(const std::string &name, const std::string &ip,
                                                     unsigned short port)
	{
		NetWorkThread &  nThread = mTaskComponent->GetNetThread();
		SocketProxy * socketProxy = new SocketProxy(nThread, name);
		TcpLocalSession * localSession = new TcpLocalSession(this, ip, port);
		this->mSessionAdressMap.emplace(socketProxy->GetSocketId(), localSession);
		return socketProxy->GetSocketId();		
	}

    void TcpClientComponent::OnDestory()
    {
    }

    TcpClientSession *TcpClientComponent::GetSession(long long id)
    {
        auto iter = this->mSessionAdressMap.find(id);
        return iter != this->mSessionAdressMap.end() ? iter->second : nullptr;
    }

    bool TcpClientComponent::CloseSession(long long id)
    {
        auto iter = this->mSessionAdressMap.find(id);
        if (iter != this->mSessionAdressMap.end())
        {
			iter->second->StartClose();           
            return true;
        }
        return false;
    }

	bool TcpClientComponent::SendByAddress(long long id, std::string * message)
	{
		TcpClientSession * tcpSession = this->GetSession(id);
		if (tcpSession == nullptr)
		{
			return false;
		}
		tcpSession->StartSendByString(message);
		return true;
	}

	bool TcpClientComponent::SendByAddress(long long id, com::DataPacket_Request & message)
	{
        std::string * data = this->Serialize(message);
        if(data== nullptr)
        {
            return false;
        }
        this->SendByAddress(id, data);
		return true;
	}

	bool TcpClientComponent::SendByAddress(long long id, com::DataPacket_Response & message)
	{
		std::string * data = this->Serialize(message);
		if (data == nullptr)
		{
			return false;
		}
		this->SendByAddress(id, data);
		return true;
	}

    std::string *TcpClientComponent::Serialize(const com::DataPacket_Request &message)
    {
        DataMessageType type = TYPE_REQUEST;
        const size_t size = message.ByteSizeLong() + 5;
        memcpy(this->mMessageBuffer, &size, sizeof(unsigned int));
        memcpy(this->mMessageBuffer + 4, &type, sizeof(char));
        if (!message.SerializeToArray(this->mMessageBuffer + 5, 1024 * 1024 - 5))
        {
            return nullptr;
        }
        return GStringPool.New(this->mMessageBuffer, size);
    }

    std::string *TcpClientComponent::Serialize(const com::DataPacket_Response &message)
    {
        DataMessageType type = TYPE_RESPONSE;
        const size_t size = message.ByteSizeLong() + 5;
        memcpy(this->mMessageBuffer, &size, sizeof(unsigned int));
        memcpy(this->mMessageBuffer + 4, &type, sizeof(char));
        if (!message.SerializeToArray(this->mMessageBuffer + 5, 1024 * 1024 - 5))
        {
            return nullptr;
        }
        return GStringPool.New(this->mMessageBuffer, size);
    }

}// namespace GameKeeper