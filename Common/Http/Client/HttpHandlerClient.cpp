//
// Created by zmhy0073 on 2022/1/19.
//

#include"HttpHandlerClient.h"
#include"Component/HttpListenComponent.h"

namespace Sentry
{
	HttpHandlerClient::HttpHandlerClient(HttpListenComponent * httpComponent, std::shared_ptr<SocketProxy> socketProxy)
		: Tcp::TcpContext(socketProxy)
	{
        this->mTimeout = 15;
		this->mHttpComponent = httpComponent;
        this->mDecodeState = Http::DecodeState::None;
    }

	void HttpHandlerClient::StartReceive(int timeout)
	{
        this->mTimeout = timeout;
        assert(this->mRecvBuffer.size() == 0);
        assert(this->mSendBuffer.size() == 0);
        this->mDecodeState = Http::DecodeState::None;       
#ifdef ONLY_MAIN_THREAD
		this->ReceiveLine();
#else
		Asio::Context & netWorkThread = this->mSocket->GetThread();
		netWorkThread.post(std::bind(&HttpHandlerClient::ReceiveLine, this));
#endif
	}

    void HttpHandlerClient::StartWriter(std::shared_ptr<Http::Response> message)
    {
#ifdef ONLY_MAIN_THREAD
        this->Send(message);
#else
        Asio::Context & netWorkThread = this->mSocket->GetThread();
        netWorkThread.post(std::bind(&HttpHandlerClient::Write, this, message));
#endif
    }

    void HttpHandlerClient::StartWriter(HttpStatus code)
    {
        std::shared_ptr<Http::Response> message = std::make_shared<Http::Response>();
        {
            message->SetCode(code);
#ifdef ONLY_MAIN_THREAD
            this->Send(message);
#else
            Asio::Context& netWorkThread = this->mSocket->GetThread();
            netWorkThread.post(std::bind(&HttpHandlerClient::Write, this, message));
#endif
        }
    }

    void HttpHandlerClient::OnTimeout(const Asio::Code & code)
    {
        if (code != asio::error::operation_aborted)
        {
            this->ClosetClient();
            this->mSocket->Close();
        }
    }

    void HttpHandlerClient::OnReceiveLine(const Asio::Code &code, std::istream &is, size_t size)
    {
        if(code && code != asio::error::eof)
        {
            this->ClosetClient();
        }
        else
        {
            if (this->mTimer == nullptr && this->mTimeout > 0)
            {
                std::chrono::seconds timeout{ this->mTimeout };
                Asio::Context& context = this->mSocket->GetThread();
                this->mTimer = std::make_shared<Asio::Timer>(context, timeout);
                this->mTimer->async_wait(std::bind(&HttpHandlerClient::OnTimeout, this, args1));
            }

            is >> this->mMethod;
            this->mHttpRequest = Http::New(this->mMethod);
            if (this->mHttpRequest == nullptr)
            {
                std::shared_ptr<Http::Response> response
                    = std::make_shared<Http::Response>();

                response->Str(HttpStatus::METHOD_NOT_ALLOWED, "unknow method");
				this->Write(response);
                return;
            }
            this->ReceiveSomeMessage();
        }
    }

    void HttpHandlerClient::OnReceiveMessage(const asio::error_code &code, std::istream & is, size_t size)
    {
        if(code && code != asio::error::eof)
        {
            this->ClosetClient();
        }
        else if(code == asio::error::eof)
        {
END_RECEIVE:
            std::shared_ptr<HttpHandlerClient> httpHandlerClient =
                std::dynamic_pointer_cast<HttpHandlerClient>(this->shared_from_this());
#ifdef ONLY_MAIN_THREAD
            this->mHttpComponent->OnRequest(httpHandlerClient);
#else
            Asio::Context &mainThread = App::Inst()->MainThread();
            mainThread.post(std::bind(&HttpListenComponent::OnRequest, this->mHttpComponent, httpHandlerClient));
#endif
        }
        else
        {
            if(!this->mHttpRequest->OnRead(is))
            {
                this->ReceiveSomeMessage();
                return;
            }
            else
            {
                this->ClearRecvStream();
                goto END_RECEIVE;
            }
        }
    }

	void HttpHandlerClient::OnSendMessage(const asio::error_code& code, std::shared_ptr<Tcp::ProtoMessage> message)
	{
		if(code)
		{
#ifdef __DEBUG__
            const std::string& address = this->mSocket->GetAddress();
			CONSOLE_LOG_ERROR("send http message error : [" << address << "]");
#endif
		}
        this->PopMessage();
        this->ClosetClient();
    }
	void HttpHandlerClient::ClosetClient()
	{
        this->mSocket->Close();
        const std::string & address = this->mSocket->GetAddress();
#ifdef ONLY_MAIN_THREAD
		this->mHttpComponent->ClosetHttpClient(address);
#else
		Asio::Context & mainThread = App::Inst()->MainThread();
		mainThread.post(std::bind(&HttpListenComponent::ClosetHttpClient, this->mHttpComponent, address));
#endif
	}
}