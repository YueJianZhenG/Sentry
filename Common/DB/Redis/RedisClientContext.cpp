//
// Created by zmhy0073 on 2022/1/15.
//
#include"RedisClientContext.h"
#include"Util/FileHelper.h"
#include"Component/Scene/LoggerComponent.h"
#include"Component/Redis/RedisComponent.h"
namespace Sentry
{
    RedisClientContext::RedisClientContext(std::shared_ptr<SocketProxy> socket,
		const RedisConfig &config, RedisComponent * component)
            : Tcp::TcpContext(socket), mConfig(config), mRedisComponent(component)
    {
        this->mSocket = socket;
        this->mConnectLock = std::make_shared<CoroutineLock>();
    }

    RedisClientContext::~RedisClientContext() noexcept
    {
        LOG_WARN("remove redis client " << this->mConfig.Name << "[" << this->mConfig.Address << "]");
    }

    XCode RedisClientContext::StartConnectAsync()
    {
        AutoCoroutineLock lock(this->mConnectLock);
        if (this->mSocket->IsOpen())
        {
            return XCode::Successful;
        }
        this->mConnectTaskSource.Clear();
#ifdef ONLY_MAIN_THREAD
        this->Connect();
#else
        this->mNetworkThread.Invoke(&RedisClientContext::Connect, this);
#endif
        return this->mConnectTaskSource.Await();
    }

    void RedisClientContext::OnConnect(const asio::error_code &error)
    {
        if (error)
        {
#ifdef __DEBUG__
            CONSOLE_LOG_ERROR(error.message());
#endif
            this->mConnectTaskSource.SetResult(XCode::NetConnectFailure);
            return;
        }
        this->mConnectTaskSource.SetResult(XCode::Successful);
    }

    void RedisClientContext::SendCommand(std::shared_ptr<RedisRequest> command)
    {
#ifdef ONLY_MAIN_THREAD
        this->AddCommandQueue(command);
#else
        this->mNetworkThread.Invoke(&RedisClientContext::AddCommandQueue, this, command);
#endif
    }

    void RedisClientContext::StartReceiveMessage()
    {
#ifdef ONLY_MAIN_THREAD
        this->StartReceive();
#else
        this->mNetworkThread.Invoke(&RedisClientContext::ReceiveLine, this);
#endif
    }

    void RedisClientContext::AddCommandQueue(std::shared_ptr<RedisRequest> command)
    {
        if (this->mCommands.empty())
        {
            this->Send(command);
        }
        this->mCommands.push(command);
        //printf("redis count = %d\n", (int)this->mCommands.size());
    }

    void RedisClientContext::OnReceiveLine(const asio::error_code &code, const std::string &buffer)
    {
        if (this->mCurResponse == nullptr)
        {
            long long taskId = 0;
            if (!this->mCommands.empty())
            {
                taskId = this->mCommands.front()->GetTaskId();
            }
            this->mCurResponse = std::make_shared<RedisResponse>(taskId);
        }
        if (code)
        {
            CONSOLE_LOG_ERROR(code.message());
        }
        int lenght = this->mCurResponse->OnRecvLine(buffer);
        if(lenght == 0)
        {
            this->OnReadComplete();
        }
        else if(lenght == -1)
        {
            this->ReceiveLine();
        }
        else
        {
            this->ReceiveMessage(lenght);
        }
    }

    void RedisClientContext::OnReceiveMessage(const asio::error_code &code, const std::string &buffer)
    {
        if (this->mCurResponse->OnRecvMessage(buffer) == 0)
        {
            this->OnReadComplete();
            return;
        }
        this->ReceiveLine();
    }

    void RedisClientContext::OnReadComplete()
    {
        std::shared_ptr<RedisResponse> response = std::move(this->mCurResponse);
#ifdef ONLY_MAIN_THREAD
        this->mRedisComponent->OnResponse(response);
#else
        SharedRedisClient redisClient = dynamic_pointer_cast<RedisClientContext>(this->shared_from_this());
        this->mNetworkThread.Invoke(&RedisComponent::OnResponse, this->mRedisComponent, redisClient, response);
#endif
        if (!this->mCommands.empty())
        {
            this->mCommands.pop();
            if (!this->mCommands.empty())
            {
                this->Send(this->mCommands.front());
            }
        }
    }

    void RedisClientContext::OnSendMessage(const asio::error_code &code, std::shared_ptr<ProtoMessage> message)
    {
        if (code)
        {
#ifdef __DEBUG__
            CONSOLE_LOG_ERROR(code.message());
#endif
            return;
        }
        this->ReceiveLine();
    }
}
