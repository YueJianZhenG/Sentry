#include"RpcClient.h"
#include"Core/App.h"
#include"Util/TimeHelper.h"
#include<iostream>
namespace GameKeeper
{
    RpcClient::RpcClient(std::shared_ptr<SocketProxy> socket, SocketType type)
            : mType(type), mSocketProxy(socket),
              mContext(socket->GetContext()),
              mNetWorkThread(socket->GetThread())
    {
        this->mIp.clear();
        this->mIsOpen = false;
        this->mIsConnect = false;
        this->mSocketId = socket->GetSocketId();
        this->mLastOperTime = Helper::Time::GetSecTimeStamp();
    }

    void RpcClient::CloseSocket(XCode code)
    {
        this->mIsOpen = false;
        this->mSocketProxy->Close();
        this->OnClose(code);
    }

    void RpcClient::Clear()
    {
        this->mSocketId = 0;
        this->mLastOperTime = 0;
        this->mSocketProxy->Close();
        this->mSocketProxy = nullptr;
    }

    void RpcClient::StartReceive()
    {
        this->mIsOpen = true;
        this->mLastOperTime = Helper::Time::GetSecTimeStamp();
        AsioTcpSocket &socket = this->mSocketProxy->GetSocket();
        unsigned short port = socket.remote_endpoint().port();
        this->mIp = socket.remote_endpoint().address().to_string();
        this->mAddress = this->mIp + ":" + std::to_string(port);
        if (mNetWorkThread.IsCurrentThread())
        {
            this->ReceiveHead();
            return;
        }
        mNetWorkThread.Invoke(&RpcClient::ReceiveHead, this);
    }

    void RpcClient::ReceiveHead()
    {
        if (!this->mSocketProxy->IsOpen())
        {
            this->CloseSocket(XCode::NetWorkError);
            return;
        }
        const size_t count = sizeof(int) + sizeof(char);
        AsioTcpSocket &socket = this->mSocketProxy->GetSocket();
        socket.async_read_some(asio::buffer(this->mReceiveBuffer, count),
                               [this](const asio::error_code &error_code, const std::size_t t)
                               {
                                   this->mLastOperTime = Helper::Time::GetSecTimeStamp();
                                   if (error_code)
                                   {
                                       this->CloseSocket(XCode::NetReceiveFailure);
                                   } else
                                   {
                                       int length = 0;
                                       char type = this->mReceiveBuffer[0];
                                       memcpy(&length, this->mReceiveBuffer + sizeof(char), sizeof(int));
                                       if (length >= MAX_DATA_COUNT)
                                       {
                                           this->CloseSocket(XCode::NetBigDataShutdown);
                                           return;
                                       }

                                       if (type == RPC_TYPE_REQUEST || type == RPC_TYPE_RESPONSE
                                           || type == RPC_TYPE_CLIENT_REQUEST)
                                       {
                                           this->ReceiveBody(type, length);
                                           return;
                                       }
                                       this->CloseSocket(XCode::UnKnowPacket);
                                   }
                               });
    }

    void RpcClient::ReceiveBody(char type, size_t size)
    {
        char *messageBuffer = this->mReceiveBuffer;
        if (size > TCP_BUFFER_COUNT)
        {
            if (size > MAX_DATA_COUNT)
            {
                this->CloseSocket(XCode::NetBigDataShutdown);
                return;
            }
            messageBuffer = new char[size];
        }
        asio::error_code code;
        this->mLastOperTime = Helper::Time::GetSecTimeStamp();
        AsioTcpSocket &nSocket = this->mSocketProxy->GetSocket();
        if (size > nSocket.available(code))
        {
            this->CloseSocket(XCode::NetReceiveFailure);
            return;
        }
        nSocket.async_read_some(asio::buffer(messageBuffer, size),
                                [this, messageBuffer, type](const asio::error_code &error_code,
                                                            const std::size_t size) {
                                    XCode code = XCode::Successful;
                                    if (error_code)
                                    {
                                        code = XCode::NetWorkError;
                                        std::cout << error_code.message() << std::endl;
                                    }
                                    switch (type)
                                    {
                                        case RPC_TYPE_REQUEST:
                                            code = this->OnRequest(messageBuffer, size);
                                            break;
                                        case RPC_TYPE_RESPONSE:
                                            code = this->OnResponse(messageBuffer, size);
                                            break;
                                        default:
                                            assert(false);
                                            break;
                                    }
                                    if (messageBuffer != this->mReceiveBuffer)
                                    {
                                        delete[] messageBuffer;
                                    }
                                    if (code == XCode::Successful)
                                    {
                                        mContext.post(std::bind(&RpcClient::ReceiveHead, this));
                                        return;
                                    }
                                    this->CloseSocket(code);
                                });
    }

    bool RpcClient::StartConnect(const std::string &ip, unsigned short port)
    {
        if(this->IsCanConnection())
        {
            this->mIsConnect = true;
            LOG_CHECK_RET_FALSE(this->mSocketProxy);
            NetWorkThread &nThread = this->mSocketProxy->GetThread();
            nThread.Invoke(&RpcClient::ConnectHandler, this, ip, port);
            return true;
        }
        return false;
    }

    void RpcClient::ConnectHandler(const std::string &ip, unsigned short port)
    {
        this->mIp = ip;
        this->mPort = port;
        AsioTcpSocket &nSocket = this->mSocketProxy->GetSocket();
        auto address = asio::ip::make_address_v4(ip);
        asio::ip::tcp::endpoint endPoint(address, port);
        LOG_DEBUG(this->mSocketProxy->GetName(), " start connect " , ip, ':', port);
        nSocket.async_connect(endPoint, [this](const asio::error_code &err)
        {
            XCode code = XCode::Failure;
            if (!err)
            {
                this->mIsOpen = true;
                code = XCode::Successful;
            }
            this->OnConnect(code);
            this->mIsConnect = false;
        });
    }

    bool RpcClient::IsCanConnection()
    {
        return this->GetSocketType() == SocketType::LocalSocket
               && !this->IsOpen() && !this->mIsConnect;
    }

    void RpcClient::SendData(char type, std::shared_ptr<Message> message)
    {
        if (!this->mSocketProxy->IsOpen() || message == nullptr)
        {
            this->CloseSocket(XCode::NetWorkError);
            this->OnSendData(XCode::NetWorkError, message);
            return;
        }
        const int body = message->ByteSize();
        const int head = sizeof(char) + sizeof(int);

        char *buffer = this->mSendBuffer;
        if (head + body > TCP_BUFFER_COUNT)
        {
            buffer = new char[head + body];
        }
        buffer[0] = type;
        memcpy(buffer + sizeof(char), &body, sizeof(int));
        if (!message->SerializePartialToArray(buffer + head, body))
        {
            if (buffer != this->mSendBuffer)
            {
                delete[]buffer;
            }
            this->OnSendData(XCode::SerializationFailure, message);
            return;
        }

        AsioTcpSocket &nSocket = this->mSocketProxy->GetSocket();
        nSocket.async_send(asio::buffer(buffer, head + body),
                           [message, this, buffer, type]
                                   (const asio::error_code &error_code, std::size_t size)
                           {
                               XCode code = XCode::Successful;
                               if (error_code)
                               {
                                   code = XCode::SendMessageFail;
                                   LOG_ERROR(error_code.message());
                                   this->CloseSocket(XCode::NetSendFailure);
                               }
                               if (buffer != this->mSendBuffer)
                               {
                                   delete[]buffer;
                               }
                               this->OnSendData(code, message);
                               this->mLastOperTime = Helper::Time::GetSecTimeStamp();
                           });
    }
}