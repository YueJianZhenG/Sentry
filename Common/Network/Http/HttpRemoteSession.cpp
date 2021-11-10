//
// Created by zmhy0073 on 2021/10/26.
//

#include "HttpRemoteSession.h"
#include <Core/App.h>
#include <Scene/RpcProtoComponent.h>
#include <Component/Scene/HttpComponent.h>
#include <Network/Http/Response/HttpGettHandler.h>
#include <Network/Http/Response/HttpPostHandler.h>
#include <Method/HttpServiceMethod.h>
namespace GameKeeper
{
    HttpRemoteSession::HttpRemoteSession(HttpComponent *component)
    {
        this->mWriterCount = 0;
        this->mHttpHandler = nullptr;
        this->mSocketProxy = nullptr;
        this->mHttpComponent = component;
        this->mHandlerMap["GET"] = new HttpGettHandler(component);
        this->mHandlerMap["POST"] = new HttpPostHandler(component);
    }

    HttpRemoteSession::~HttpRemoteSession() noexcept
    {
        auto iter = this->mHandlerMap.begin();
        for(; iter != this->mHandlerMap.end(); iter++)
        {
            delete iter->second;
        }
        this->mHandlerMap.clear();
    }

    void HttpRemoteSession::Clear()
    {
        HttpSessionBase::Clear();

        delete this->mSocketProxy;
        this->mWriterCount = 0;
        this->mSocketProxy = nullptr;
        this->mHttpHandler = nullptr;
        this->mMethod.clear();
        this->mAddress.clear();
    }

    bool HttpRemoteSession::WriterToBuffer(std::ostream & os)
    {
        if(this->mHttpHandler == nullptr)
        {
            HttpStatus code = HttpStatus::METHOD_NOT_ALLOWED;
            os << HttpVersion << (int) code << " " << HttpStatusToString(code) << "\r\n";
            os << "Server: " << "GameKeeper" << "\r\n";
            os << "Connection: " << "close" << "\r\n\r\n";
            return true;
        }
        return this->mHttpHandler->WriterToBuffer(os);
    }

    void HttpRemoteSession::Start(SocketProxy *socketProxy)
    {
        delete this->mSocketProxy;
        this->mSocketProxy = socketProxy;
        this->StartReceiveHead();
    }

	void HttpRemoteSession::OnReceiveHeard(asio::streambuf & streamBuf)
    {
        std::istream is(&streamBuf);
        is >> this->mMethod;
        this->mHttpHandler = nullptr;
        auto iter = this->mHandlerMap.find(this->mMethod);
        if (iter == this->mHandlerMap.end())
        {
            this->StartSendHttpMessage();
            GKDebugError("not find http method " << this->mMethod);
            return;
        }
        this->mHttpHandler = iter->second;
        if(!this->mHttpHandler->OnReceiveHead(streamBuf))
        {
            this->mHttpHandler->SetResponseCode(HttpStatus::BAD_REQUEST);
            this->StartSendHttpMessage();
        }
        else
        {
            const std::string & method = this->mHttpHandler->GetMethod();
            const std::string & service = this->mHttpHandler->GetComponent();
            if(this->mHttpComponent->GetHttpMethod(service, method) == nullptr)
            {
                this->mHttpHandler->SetResponseCode(HttpStatus::NOT_FOUND);
                this->StartSendHttpMessage();
            }
        }
    }

    void HttpRemoteSession::SetCode(XCode code)
    {
        this->mCode = code;
        CoroutineComponent * corComponent = App::Get().GetCorComponent();
        MainTaskScheduler & taskScheduler = App::Get().GetTaskScheduler();
        taskScheduler.AddMainTask(&HttpComponent::OnRequest, this->mHttpComponent, this);
    }

    void HttpRemoteSession::OnReceiveHeadAfter(XCode code)
    {
        if(code != XCode::Successful || this->mHttpHandler == nullptr)
        {
            this->SetCode(code);
        }
        else if(this->mHttpHandler->GetType() == HttpMethodType::GET)
        {
            this->SetCode(XCode::Successful);
        }
        else
        {
            this->StartReceiveBody();
        }
    }

    void HttpRemoteSession::StartReceiveBody()
    {
        asio::error_code code;   
		GKAssertRet_F(this->mSocketProxy->IsOpen());
		AsioTcpSocket &socket = this->mSocketProxy->GetSocket();
        if (socket.available(code) == 0)
        {
            this->SetCode(XCode::Successful);
        }
        else
        {
            asio::async_read(socket, this->mStreamBuf, asio::transfer_at_least(1),
                             std::bind(&HttpRemoteSession::ReadBodyCallback, this, args1, args2));
        }
    }

    void HttpRemoteSession::ReadBodyCallback(const asio::error_code &err, size_t size)
    {
        if(err == asio::error::eof)
        {
            this->SetCode(XCode::Successful);
        }
        else if(err)
        {
            this->SetCode(XCode::NetReceiveFailure);
        }
        else
        {
            if(this->mHttpHandler->OnReceiveBody(this->mStreamBuf))
            {
                AsioContext &context = this->mSocketProxy->GetContext();
                context.post(std::bind(&HttpRemoteSession::StartReceiveBody, this));
            }
        }
    }

    void HttpRemoteSession::OnWriterAfter(XCode code)
    {
#ifdef __DEBUG__
        if (this->mHttpHandler != nullptr)
        {
            long long endTime = TimeHelper::GetMilTimestamp();
            GKDebugLog("http call " << this->mHttpHandler->GetComponent() << "."
                                    << this->mHttpHandler->GetMethod() << " use time = "
                                    << ((endTime - this->mHttpHandler->GetStartTime()) / 1000.0f) << "s");
        }
#endif
        if(this->mHttpHandler != nullptr)
        {
            this->mHttpHandler->Clear();
        }
        delete this;
    }
}