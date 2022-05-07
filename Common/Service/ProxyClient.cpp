//
// Created by zmhy0073 on 2022/1/13.
//
#include"ProxyClient.h"
#include"Util/StringHelper.h"
#include"Component/Logic/RegistryService.h"
namespace Sentry
{
    ProxyClient::ProxyClient(const std::string &name, const std::string &address)
    {
        this->mIsClose = false;
        this->mAddress = address;
        this->mServiceName = name;
        this->mState = NodeState::None;
        this->mTaskComponent = App::Get()->GetTaskComponent();
        this->mTaskComponent->Start(&ProxyClient::SendFromQueue, this);
        this->mRpcCliemComponent = App::Get()->GetComponent<RpcClientComponent>();
    }

    bool ProxyClient::IsConnected()
    {
        if (this->mNodeClient == nullptr)
        {
            this->mNodeClient = this->mRpcCliemComponent->GetOrCreateSession
					(this->mAddress);
        }
        if (this->mNodeClient->GetSocketProxy()->IsOpen())
        {
            return true;
        }
        for (int index = 0; index < 3; index++)
        {
            if(this->mNodeClient->StartConnectAsync())
            {
                return true;
            }
            this->mTaskComponent->Sleep(1000);
            LOG_ERROR("connect " << this->mAddress << " failure count = " << index);
        }
        return false;
    }

    void ProxyClient::SendFromQueue()
    {
        this->mLoopTaskSource = std::make_shared<LoopTaskSource>();
        while (!this->mIsClose)
        {
            if (this->mMessageQueue.empty())
            {
                this->mLoopTaskSource->Await();
            }
            if (!this->IsConnected())
            {
                return;
            }
            while (!this->mMessageQueue.empty())
            {
                auto requestMessage = this->mMessageQueue.front();
                this->mMessageQueue.pop();
                this->mNodeClient->SendToServer(requestMessage);
            }
        }
        this->mRpcCliemComponent->StartClose(this->mNodeClient->GetAddress());
    }

    bool ProxyClient::OnConnectFailure(int count)
    {
        if (count <= 3)
        {
            return true;
        }
        return false;
    }

    std::shared_ptr<com::Rpc_Request> ProxyClient::PopMessage()
    {
        if(this->mMessageQueue.empty())
        {
            return nullptr;
        }
        auto res = this->mMessageQueue.front();
        this->mMessageQueue.pop();
        return res;
    }

    void ProxyClient::PushMessage(std::shared_ptr<com::Rpc_Request> message)
    {
        this->mMessageQueue.emplace(message);
        if(this->mLoopTaskSource != nullptr)
        {
            this->mLoopTaskSource->SetResult();
        }
    }
}
