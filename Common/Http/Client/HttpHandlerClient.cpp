//
// Created by zmhy0073 on 2022/1/19.
//

#include"HttpHandlerClient.h"

#include"Entity/Actor/App.h"
#include"Http/Common/HttpRequest.h"
#include"Http/Common/HttpResponse.h"
#include"Http/Component/HttpListenComponent.h"

namespace Tendo
{
	HttpHandlerClient::HttpHandlerClient(HttpListenComponent * httpComponent, std::shared_ptr<Tcp::SocketProxy> socketProxy)
		: Tcp::TcpContext(std::move(socketProxy))
	{
        this->mTimeout = 15;
		this->mHttpComponent = httpComponent;
    }

	void HttpHandlerClient::StartReceive(int timeout)
	{
        this->mTimeout = timeout;
        assert(this->mRecvBuffer.size() == 0);
        assert(this->mSendBuffer.size() == 0);
#ifdef ONLY_MAIN_THREAD
		this->ReceiveLine();
#else
		Asio::Context & netWorkThread = this->mSocket->GetThread();
		netWorkThread.post(std::bind(&HttpHandlerClient::ReceiveLine, this));
#endif
	}

    void HttpHandlerClient::StartWriter(const std::shared_ptr<Http::IResponse>& message)
    {
#ifdef ONLY_MAIN_THREAD
        this->Write(message);
#else
        Asio::Context & netWorkThread = this->mSocket->GetThread();
        netWorkThread.post(std::bind(&HttpHandlerClient::Write, this, message));
#endif
    }

    void HttpHandlerClient::StartWriter(HttpStatus code)
    {
        std::shared_ptr<Http::DataResponse> message = std::make_shared<Http::DataResponse>();
        {
            message->SetCode(code);
#ifdef ONLY_MAIN_THREAD
            this->Write(message);
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
            //this->mSocket->Close();
        }
    }

    void HttpHandlerClient::OnReceiveLine(const Asio::Code &code, std::istream &is, size_t size)
    {
        if((code && code != asio::error::eof) || size == 0)
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
			if(this->mHttpRequest == nullptr)
			{
				is >> this->mMethod;
				const std::string& from = this->mSocket->GetAddress();
				this->mHttpRequest = Http::New(this->mMethod, from);
				if (this->mHttpRequest == nullptr)
				{
					this->OnComplete(HttpStatus::METHOD_NOT_ALLOWED);
					return;
				}
			}
			this->OnReadLater(this->mHttpRequest->OnRead(is));
        }
    }

	void HttpHandlerClient::OnComplete(HttpStatus status)
	{
		this->ClearRecvStream();
		this->ClearSendStream();
		if(status != HttpStatus::OK)
		{
			std::shared_ptr<Http::DataResponse> response
					= std::make_shared<Http::DataResponse>();
			response->SetCode(status);
			this->Write(response);
		}
		else
		{
			this->mHttpRequest->OnComplete();
			if (this->mTimer != nullptr)
			{
				this->mTimeout = 0;
				asio::error_code code;
				this->mTimer->cancel(code);
				this->mTimer = nullptr;
			}
#ifdef ONLY_MAIN_THREAD
			this->mHttpComponent->OnRequest(this->mHttpRequest);
#else
			Asio::Context& io = App::Inst()->MainThread();
			io.post(std::bind(&HttpListenComponent::OnRequest, this->mHttpComponent, this->mHttpRequest));
#endif
			this->mHttpRequest = nullptr;
		}
	}

    void HttpHandlerClient::OnReceiveMessage(const asio::error_code &code, std::istream & is, size_t size)
	{
		if (code && code != asio::error::eof)
		{
			this->ClosetClient();
			return;
		}
		if (code == asio::error::eof)
		{
			this->OnComplete(HttpStatus::OK);
			return;
		}
		this->OnReadLater(this->mHttpRequest->OnRead(is));
	}

	void HttpHandlerClient::OnSendMessage(const asio::error_code& code, std::shared_ptr<Tcp::ProtoMessage> message)
	{
		if(code)
		{
#ifdef __DEBUG__
            const std::string& address = this->mSocket->GetAddress();
			CONSOLE_LOG_ERROR("send http message error : [" << address << "] " << code.message());
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

	void HttpHandlerClient::OnReadLater(int num)
	{
		switch(num)
		{
			case HTTP_READ_LINE:
				this->ReceiveLine();
				break;
			case HTTP_READ_SOME:
				this->ReceiveSomeMessage();
				break;
			case HTTP_READ_ERROR:
				this->OnComplete(HttpStatus::BAD_REQUEST);
				break;
			case HTTP_READ_COMPLETE:
				this->OnComplete(HttpStatus::OK);
				break;
			default:
				this->ReceiveMessage(num);
				break;
		}
	}
}