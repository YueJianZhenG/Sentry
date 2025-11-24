//
// Created by zmhy0073 on 2022/1/19.
//

#include"HttpSession.h"
#include"XCode/XCode.h"
#include"Util/Tools/TimeHelper.h"

namespace http
{
	Session::Session(Component* component, Asio::Context& io)
			: tcp::Client(1024), mSockId(0), mComponent(component), mMainContext(io)
	{
#ifdef __DEBUG__
		this->mStartTime = 0;
#endif
	}

	Session::~Session() noexcept = default;

	void Session::StartReceiveBody(std::unique_ptr<http::Content> content, int timeout)
	{
		this->mRequest->SetContent(std::move(content));
#ifdef ONLY_MAIN_THREAD
		this->ReadSome();
#else
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, self = this->shared_from_this(), timeout] {
			this->StartTimer(timeout, tcp::timeout::read);
			this->ReadSome();
		});
#endif
	}

	void Session::StartReceive(int id, tcp::Socket* socket, int timeout)
	{
		this->mSockId = id;
		this->SetSocket(socket);
#ifdef __DEBUG__
		this->mStartTime = help::Time::NowMil();
#endif
#ifdef ONLY_MAIN_THREAD
		this->ReadLine();
#else
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, timeout, self = this->shared_from_this()]
		{
			this->Clear();
			this->ReadLine();
			this->StartTimer(timeout, tcp::timeout::read);
		});
#endif
	}

	void Session::StartClose(int code)
	{
#ifdef ONLY_MAIN_THREAD
		this->ClosetClient(code);
#else
		Asio::Context & ctx = this->mSocket->GetContext();
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(ctx, [this, code, self] { this->ClosetClient(code); });
#endif
	}

	bool Session::StartWriter(HttpStatus status, int timeout)
	{
		this->mResponse->SetCode(status);
		return this->StartWriter(timeout);
	}

	bool Session::StartWriter(HttpStatus status, std::unique_ptr<Content> data, int timeout)
	{
		this->mResponse->SetCode(status);
		this->mResponse->SetContent(data);
		return this->StartWriter(timeout);
	}

	bool Session::StartWriter(int timeout)
	{
		if (this->mSocket == nullptr)
		{
			this->mComponent->OnClientError(this->mSockId, XCode::NetWorkError);
			return false;
		}

#ifdef ONLY_MAIN_THREAD
		this->Write(*this->mResponse, timeout);
#else
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(this->mSocket->GetContext(), [this, timeout, self]
		{
			if(this->mResponse->IsOk()
					&& this->mRequest->Header().IsKeepAlive())
			{
				this->mResponse->Header().SetKeepAlive(5);
			}

			if (this->mResponse->GetBody() == nullptr)
			{
				this->mResponse->Header().Add(http::Header::ContentLength, 0);
			}
			this->Write(*this->mResponse, timeout);
		});
#endif
		return true;
	}

	void Session::OnReadError(const Asio::Code& code)
	{
		switch (code.value())
		{
			case asio::error::eof:
			case asio::error::operation_aborted:
				this->ClosetClient(XCode::Ok);
				break;
			case asio::error::timed_out:
				this->ClosetClient(XCode::NetTimeout);
				break;
			default:
				this->ClosetClient(XCode::NetReadFailure);
				break;
		}
	}

	void Session::OnReceiveLine(std::istream& is, size_t size)
	{
		asio::error_code code;
		this->OnReceiveMessage(is, size, code);
	}

	void Session::OnReadPause()
	{
		if (this->mRequest->Header().Count() == 0)
		{
			this->OnComplete(HttpStatus::BAD_REQUEST);
			return;
		}
		this->StopTimer();
		this->mResponse = std::make_unique<http::Response>();

		http::Request * request = this->mRequest.get();
		http::Response * response = this->mResponse.get();
#ifdef ONLY_MAIN_THREAD
		this->mComponent->OnReadHead(request, response);
#else
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(this->mMainContext, [this, self, request, response]
		{
			this->mComponent->OnReadHead(request, response);
		});
#endif
	}

	void Session::OnComplete(HttpStatus status)
	{
		this->StopTimer();
		if (status != HttpStatus::OK)
		{
			if(this->mResponse == nullptr)
			{
				this->mResponse = std::make_unique<http::Response>();
			}
			this->mResponse->SetCode(status);
			this->mResponse->Header().SetClose();
			this->Write(*this->mResponse, 5);
		}
		else
		{
			http::Request * request = this->mRequest.get();
			http::Response * response = this->mResponse.get();
#ifdef ONLY_MAIN_THREAD
			this->mComponent->OnMessage(this->mSockId, request, response);
#else

			std::shared_ptr<Client> self = this->shared_from_this();
			asio::post(this->mMainContext, [this, self, request, response]
			{
				this->mComponent->OnMessage(this->mSockId, request, response);
			});
#endif
		}
	}

	void Session::OnReceiveMessage(std::istream& is, size_t size, const Asio::Code&)
	{
		if(this->mRequest == nullptr)
		{
			this->mRequest = std::make_unique<http::Request>();
			this->mRequest->SetSockId(this->mSockId);
#ifdef __DEBUG__
			this->mRequest->Header().Add("t", this->mStartTime);
#endif
			const std::string& ip = this->mSocket->GetIp();
			this->mRequest->Header().Set(http::Header::RealIp, ip);
		}
		int flag = this->mRequest->OnRecvMessage(is, size);
		if (flag > 0)
		{
			this->ReadLength(flag);
			return;
		}
		switch (flag)
		{
			case tcp::read::done:
				this->OnComplete(HttpStatus::OK);
				break;
			case tcp::read::line:
				this->ReadLine();
				break;
			case tcp::read::some:
				this->ReadSome();
				break;
			case tcp::read::error:
			case tcp::read::decode_error:
				this->OnComplete(HttpStatus::BAD_REQUEST);
				break;
			case tcp::read::pause:
				this->OnReadPause();
				break;
			case tcp::read::big_long:
				this->OnComplete(HttpStatus::PAYLOAD_TOO_LARGE);
				break;
			case tcp::read::all:
				this->ReadAll();
				break;
			default:
				this->ClosetClient(XCode::Failure);
				break;
		}
	}

	void Session::OnSendMessage(size_t size)
	{
		this->StopTimer();
		if (this->mResponse->Header().IsKeepAlive())
		{
			this->Clear();
			this->ReadLine(10);
			return;
		}
		this->ClosetClient(XCode::Ok);
	}

	void Session::OnSendMessage(const asio::error_code& error)
	{
		if(error == asio::error::timed_out)
		{
			this->ClosetClient(XCode::NetTimeout);
			return;
		}
		this->ClosetClient(XCode::SendMessageFail);
	}

	void Session::Clear()
	{
		this->StopTimer();
		this->ClearBuffer();
		this->mRequest.reset();
		this->mResponse.reset();
	}

	void Session::ClosetClient(int code)
	{
		this->mSocket->Close();
		this->Clear();
#ifdef ONLY_MAIN_THREAD
		this->mComponent->OnClientError(this->mSockId, code);
#else
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(this->mMainContext, [this, self, code]()
		{
			this->mComponent->OnClientError(this->mSockId, code);
		});
#endif
	}
}