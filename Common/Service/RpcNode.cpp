﻿#include"RpcNode.h"
#include<Core/App.h>
#include<Coroutine/TaskComponent.h>
#include<Async/RpcTask/RpcTask.h>
#include<Rpc/RpcClientComponent.h>
#include<Util/StringHelper.h>
#include<Scene/RpcConfigComponent.h>
#include"Rpc/RpcComponent.h"
#include"Service/NodeHelper.h"
namespace GameKeeper
{
	RpcNode::RpcNode(int id)
		: mGlobalId(id), mIsClose(false), mSocketId(0)
	{
        this->mCallHelper = new NodeHelper(id);
        this->mRpcComponent = App::Get().GetComponent<RpcComponent>();
        this->mRpcClientComponent = App::Get().GetComponent<RpcClientComponent>();
        LOG_CHECK_RET(this->mCorComponent = App::Get().GetComponent<TaskComponent>());
        LOG_CHECK_RET(this->mRpcConfigComponent = App::Get().GetComponent<RpcConfigComponent>());
	}

	bool RpcNode::AddService(const std::string &service)
	{
		auto iter = this->mServiceArray.find(service);
		if (iter == this->mServiceArray.end())
		{
			this->mServiceArray.insert(service);
			return true;
		}
		return false;
	}

    bool RpcNode::UpdateNodeProxy(const s2s::NodeInfo &nodeInfo)
    {
        auto iter = nodeInfo.listeners().find("rpc");
        if(iter == nodeInfo.listeners().end())
        {
            return false;
        }

        this->mServiceArray.clear();
        this->mAreaId = nodeInfo.areaid();
        this->mNodeId = nodeInfo.nodeid();
        this->mNodeIp = nodeInfo.serverip();
        this->mNodeName = nodeInfo.servername();
        this->mNodePort = (unsigned short)iter->second;
        for(const std::string & name : nodeInfo.services())
        {
            this->mServiceArray.insert(name);
        }
        this->mNodeInfo.CopyFrom(nodeInfo);
        return true;
    }

    void RpcNode::ConnectToNode()
    {
        auto rpcClient = this->mRpcClientComponent->GetRpcSession(this->mSocketId);
        if(rpcClient == nullptr)
        {
            rpcClient = this->mRpcClientComponent->NewSession(this->mNodeName);
        }
        if(!rpcClient->IsOpen())
        {
            this->mSocketId = rpcClient->GetSocketProxy().GetSocketId();
            auto method = NewMethodProxy(&RpcNode::OnConnectAfter, this);
            rpcClient->StartConnect(this->mNodeIp, this->mNodePort, method);
        }
    }

    void RpcNode::OnConnectAfter()
    {
        while(!this->mWaitSendQueue.empty())
        {
            auto message  = this->mWaitSendQueue.front();
            if(!this->mRpcClientComponent->SendByAddress(this->mSocketId, message))
            {
                break;
            }
            this->mWaitSendQueue.pop();
        }
    }

    void RpcNode::Destory()
    {
        size_t size = this->mWaitSendQueue.size();
        this->mRpcClientComponent->CloseSession(this->mSocketId);
        if(size > 0)
        {
            LOG_ERROR("send queue has " << size << " data");
        }
        delete this;
    }

    bool RpcNode::HasService(const std::string &service)
	{
		auto iter = this->mServiceArray.find(service);
		return iter != this->mServiceArray.end();
	}

    void RpcNode::GetServices(std::vector<std::string> & services)
    {
        services.clear();
        for(const std::string & name : this->mServiceArray)
        {
            services.emplace_back(name);
        }
    }

	com::Rpc_Request * RpcNode::NewRequest(const std::string & method)
	{
		auto config = this->mRpcConfigComponent->GetProtocolConfig(method);
		if (config == nullptr)
		{
			return nullptr;
		}
        auto request = new com::Rpc_Request();
		request->set_methodid(config->MethodId);
        if(!this->mRpcClientComponent->SendByAddress(this->mSocketId, request))
        {
            this->ConnectToNode();
            this->mWaitSendQueue.push(request);
        }
        return request;
	}

    std::shared_ptr<RpcTask> RpcNode::NewRpcTask(const std::string &method)
    {
        auto requestData = this->NewRequest(method);
        if (requestData == nullptr)
        {
            LOG_ERROR("not find rpc config " << method);
            return std::make_shared<RpcTask>(XCode::NotFoundRpcConfig);
        }
        std::shared_ptr<RpcTask> rpcTask(new RpcTask(requestData->methodid()));
        requestData->set_rpcid(rpcTask->GetTaskId());
        return rpcTask;
    }

    std::shared_ptr<RpcTask> RpcNode::NewRpcTask(const std::string &method, const Message &message)
    {
        auto requestData = this->NewRequest(method);
        if (requestData == nullptr)
        {
            LOG_ERROR("not find rpc config " << method);
            return std::make_shared<RpcTask>(XCode::NotFoundRpcConfig);
        }
        std::shared_ptr<RpcTask> rpcTask(new RpcTask(requestData->methodid()));

        requestData->set_rpcid(rpcTask->GetTaskId());
        requestData->mutable_data()->PackFrom(message);
        return rpcTask;
    }
}// namespace GameKeeper