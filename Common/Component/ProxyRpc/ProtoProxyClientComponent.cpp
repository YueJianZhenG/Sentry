//
// Created by mac on 2021/11/28.
//

#include "ProtoProxyClientComponent.h"
#include"Core/App.h"
#include"Network/Rpc/RpcProxyClient.h"
#include"ProxyRpc/ProtoProxyComponent.h"
#include"ProtoRpc/ProtoRpcComponent.h"
#ifdef __DEBUG__
#include"Util/StringHelper.h"
#include"Scene/RpcConfigComponent.h"
#include"google/protobuf/util/json_util.h"
#endif
namespace GameKeeper
{
    bool ProtoProxyClientComponent::Awake()
    {
        this->mTimerComponent = App::Get().GetTimerComponent();
        this->mRpcComponent = this->GetComponent<ProtoRpcComponent>();
        this->mProxyComponent = this->GetComponent<ProtoProxyComponent>();
        return true;
    }

    void ProtoProxyClientComponent::OnListen(SocketProxy *socket)
    {
        long long id = socket->GetSocketId();
        auto iter = this->mProxyClientMap.find(id);
        LOG_CHECK_RET(iter == this->mProxyClientMap.end());
        const std::string ip = socket->GetSocket().remote_endpoint().address().to_string();
        if(this->mBlackList.find(ip) == this->mBlackList.end())
        {
            auto rpcClient = new RpcProxyClient(socket, LocalSocket, this);
#ifdef __DEBUG__
            LOG_INFO("new player connect proxy component ip : " << ip);
#endif
            rpcClient->StartReceive();
            this->mProxyClientMap.insert(std::make_pair(id, rpcClient));
            //this->mTimerComponent->AsyncWait(5000, &ProtoProxyClientComponent::CheckPlayerLogout, this, id);
            return;
        }
        delete socket;
    }

    void ProtoProxyClientComponent::OnRequest(c2s::Rpc_Request *request) //客户端调过来的
    {
#ifdef __DEBUG__
        std::string json;
        util::MessageToJsonString(*request, &json);
        LOG_WARN("**********[client request]**********");
        LOG_WARN("func = " << request->methodname());
        LOG_WARN("json = " << json);
        LOG_WARN("*****************************************");
#endif

        LocalObject<c2s::Rpc_Request> local(request);
        XCode code = this->mProxyComponent->OnRequest(request);
        if(code != XCode::Successful)
        {
            auto responseMessage = new c2s::Rpc_Response();
#ifdef __DEBUG__
            auto configCom = App::Get().GetComponent<RpcConfigComponent>();
            LOG_ERROR("player call " << request->methodname() << " failure "
                                     << "error = " << configCom->GetCodeDesc(code));
#endif
            responseMessage->set_code(code);
            responseMessage->set_rpcid(request->rpcid());
            this->SendProtoMessage(request->sockid(), responseMessage);
        }
    }

    void ProtoProxyClientComponent::OnCloseSocket(long long id, XCode code)
    {
        auto iter = this->mProxyClientMap.find(id);
        if(iter != this->mProxyClientMap.end())
        {
            RpcProxyClient *proxyClient = iter->second;
            this->mProxyClientMap.erase(iter);
            if (code == XCode::UnKnowPacket) //恶意消息
            {
                this->mBlackList.emplace(proxyClient->GetIp());
            }
            delete proxyClient;
#ifdef __DEBUG__
            auto configCom = App::Get().GetComponent<RpcConfigComponent>();
            LOG_WARN("remove player session code = " << configCom->GetCodeDesc(code));
#endif
        }
    }

    bool ProtoProxyClientComponent::SendProtoMessage(long long sockId, const c2s::Rpc_Response *message)
    {
        auto proxyClient = this->GetProxyClient(sockId);
        if(proxyClient == nullptr || !proxyClient->IsOpen())
        {
            return false;
        }
        return proxyClient->StartSendData(RPC_TYPE_RESPONSE, message);
    }

    RpcProxyClient *ProtoProxyClientComponent::GetProxyClient(long long int sockId)
    {
        auto iter = this->mProxyClientMap.find(sockId);
        return iter != this->mProxyClientMap.end() ? iter->second : nullptr;
    }

    void ProtoProxyClientComponent::StartClose(long long int id)
    {
        RpcProxyClient * proxyClient = this->GetProxyClient(id);
        if(proxyClient != nullptr)
        {
            proxyClient->StartClose();
        }
    }

    void ProtoProxyClientComponent::CheckPlayerLogout(long long sockId)
    {
        RpcProxyClient * proxyClient = this->GetProxyClient(sockId);
        if(proxyClient != nullptr)
        {
            long long nowTime = TimeHelper::GetSecTimeStamp();
            if(nowTime - proxyClient->GetLastOperatorTime() >= 5)
            {
                proxyClient->StartClose();
                LOG_ERROR(sockId << " logout ");
                return;
            }
        }
        this->mTimerComponent->AsyncWait(5000, &ProtoProxyClientComponent::CheckPlayerLogout, this, sockId);
    }

}