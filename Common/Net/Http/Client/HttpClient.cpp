//
// Created by yjz on 2022/1/19.
//
#include <regex>
#include "HttpClient.h"
#include "Util/Tools/Guid.h"

namespace http
{
	Client::Client(Component* httpComponent, Asio::Context & io)
			: tcp::Client(1024), mComponent(httpComponent), mMainContext(io)
	{
		this->mSockId = 0;
	}

	Client::Client(http::Client::Component* component, tcp::Socket* socket, Asio::Context& io)
			: tcp::Client(0), mComponent(component), mMainContext(io)
	{
		this->mSockId = 0;
		this->SetSocket(socket);
	}

	void Client::Do(std::unique_ptr<http::Request>& request,
			std::unique_ptr<http::Response>& response, int taskId)
	{
		this->mSockId = taskId;
		this->mRequest = std::move(request);
		this->mResponse = std::move(response);
#ifdef ONLY_MAIN_THREAD
		const http::Url &url = this->mRequest->GetUrl();
		this->Connect(url.Host(), url.Port(), 5);
#else
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, self = this->shared_from_this()]
		{
			const http::Url& url = this->mRequest->GetUrl();
			this->Connect(url.Host(), url.Port(), this->mRequest->Timeout());
		});
#endif
	}

	bool Client::SyncSend(const std::unique_ptr<http::Request>& request)
	{
		const http::Url& url = request->GetUrl();
		if (!this->ConnectSync(url.Host(), url.Port()))
		{
			return false;
		}
		return this->SendSync(*request);
	}

	bool Client::SyncSend(const std::unique_ptr<http::Request>& request, http::Response& response)
	{
		const http::Url& url = request->GetUrl();
		if (!this->ConnectSync(url.Host(), url.Port()))
		{
			return false;
		}
		if (!this->SendSync(*request))
		{
			return false;
		}

		size_t size = 0;
		std::istream is(&this->mRecvBuffer);
		if (!this->RecvLineSync(size))
		{
			return false;
		}
		int flag = response.OnRecvMessage(is, size);
		while (true)
		{
			switch (flag)
			{
				case tcp::read::error:
				case tcp::read::decode_error:
					return false;
				case tcp::read::line:
					flag = this->RecvLineSync(size);
					break;
				case tcp::read::some:
				{
					size_t count = this->mSocket->CanRecvCount();
					if (count > 0)
					{
						flag = this->RecvSync(count, size);
						break;
					}
					return false;
				}
				case tcp::read::done:
					return true;
				default:
					flag = this->RecvSync(flag, size);
					break;
			}
			response.OnRecvMessage(is, size);
		}
		return true;
	}

	void Client::OnConnect(const Asio::Code & code, int count)
	{
		if (code.value() != Asio::OK)
		{
			this->OnComplete(HttpStatus::SERVICE_UNAVAILABLE);
		}
		else
		{
			this->StopTimer();
			if (this->mRequest->Header().IsKeepAlive())
			{
				this->mSocket->SetOption(tcp::OptionType::KeepAlive, true);
			}
			this->Write(*this->mRequest);
		}
	}

	void Client::OnSendMessage(size_t size)
	{
		if(this->mSocket->IsActive())
		{
			this->ReadLine(this->mRequest->Timeout());
		}
	}

	void Client::OnSendMessage(const asio::error_code &code)
	{
		this->OnComplete(HttpStatus::INTERNAL_SERVER_ERROR);
	}

	void Client::OnComplete(HttpStatus code)
	{
		if(!this->mSocket->IsActive())
		{
			return;
		}
		this->StopTimer();
		this->mSocket->Close();
		this->mResponse->SetCode(code);
		http::Content* httpData = const_cast<http::Content*>(this->mResponse->GetBody());
		if (httpData != nullptr)
		{
			httpData->OnDecode();
		}
		if (this->mComponent == nullptr)
		{
			return;
		}
		http::Request* request = this->mRequest.release();
		http::Response* response = this->mResponse.release();
#ifdef ONLY_MAIN_THREAD
		this->mComponent->OnMessage(this->mSockId, request, response);
#else
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(this->mMainContext, [this, self, request, response]
		{
			this->mComponent->OnMessage(this->mSockId, request, response);
		});
#endif
	}

	void Client::OnReadError(const Asio::Code &code)
	{
		HttpStatus status = HttpStatus::INTERNAL_SERVER_ERROR;
		if(code == asio::error::eof)
		{
			status = HttpStatus::OK;
		}
		else if(code == asio::error::timed_out)
		{
			status = HttpStatus::REQUEST_TIMEOUT;
		}
		if(this->mRecvBuffer.size() > 0)
		{
			std::istream steam(&this->mRecvBuffer);
			this->OnReceiveMessage(steam, this->mRecvBuffer.size(), code);
		}
		this->OnComplete(status);
	}

	void Client::OnReceiveLine(std::istream &is, size_t size)
	{
		if(this->mSocket->IsActive())
		{
			asio::error_code code;
			this->OnReceiveMessage(is, size, code);
		}
	}

	void Client::OnReceiveMessage(std::istream &is, size_t size, const Asio::Code &)
	{
		int flag = this->mResponse->OnRecvMessage(is, size);
		switch (flag)
		{
		case tcp::read::done: // 完成
			this->OnComplete(HttpStatus::OK);
			break;
		case tcp::read::line: // 读一行
			this->ReadLine();
			break;
		case tcp::read::some: // 读一些
			this->ReadSome();
			break;
		case tcp::read::error: // 消息错误
			this->OnComplete(HttpStatus::INTERNAL_SERVER_ERROR);
			break;
		case tcp::read::big_long:
		case tcp::read::decode_error: // 解析错误
			this->OnComplete(HttpStatus::INTERNAL_SERVER_ERROR);
			break;
		default:
			this->ReadLength(flag);
			break;
		}
	}
}
