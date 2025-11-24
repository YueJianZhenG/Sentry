//
// Created by 64658 on 2025/1/2.
//
#include "XCode/XCode.h"
#include "WsSession.h"

#include <strstream>

#include "Http/Common/HttpRequest.h"
#include "Http/Common/HttpResponse.h"
#include "Util/Tools/String.h"
#include "Util/Crypt/sha1.h"
#include "Proto/Bson/base64.h"
namespace ws
{
	std::string generateSecWebSocketAccept(const std::string& secWebSocketKey) {
		const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		std::string keyConcat = secWebSocketKey + GUID;

		std::string hash = help::Sha1::GetHash(keyConcat);
		return _bson::base64::encode(hash);
	}
}

namespace ws
{

	Session::Session(int id, Component * component, Asio::Context & main, char msg)
		: tcp::Client(1024 * 10), mSockId(id), mIsHttp(true), mComponent(component), mMainContext(main)
	{
		this->mMsg = msg;
		this->mHttpRequest = nullptr;
	}

	Session::~Session() noexcept
	{
		delete this->mHttpRequest;
	}

	void Session::Stop()
	{
		Asio::Context & context = this->mSocket->GetContext();
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(context, [self, this]() { this->Close(XCode::CloseSocket); });
	}

	void Session::StartReceive(tcp::Socket* tcpSocket)
	{
		this->SetSocket(tcpSocket);
		Asio::Context & context = tcpSocket->GetContext();
		asio::post(context, [this, self = this->shared_from_this()]() {
			this->ReadLine();
			this->StartTimer(5, tcp::timeout::read);
		});
	}

	void Session::Send(std::unique_ptr<rpc::Message>& req)
	{
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, self = this->shared_from_this(), message = req.release()] ()
		{
			std::unique_ptr<ws::Message> wsMessage = std::make_unique<ws::Message>();
			if(message->GetType() == rpc::type::close)
			{
				wsMessage->SetBody(ws::OPCODE_CLOSE, "close", false);
				this->AddToSendQueue(wsMessage);
			}
			else if(message->GetType() == rpc::type::parameter)
			{
				const rpc::Head & head = message->GetHead();
				for(const std::pair<std::string,std::string> & iter : head.GetValue())
				{
					this->mParameter.emplace_back(iter.first, iter.second);
				}
			}
			else
			{
				this->mStream.str("");
				message->SetMsg(this->mMsg);
				message->OnSendMessage(this->mStream);
				wsMessage->SetBody(ws::OPCODE_BIN, this->mStream.str(), false);

				this->AddToSendQueue(wsMessage);
			}
			delete message;
		});
	}

	void Session::Send(std::unique_ptr<ws::Message> & req)
	{
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, self = this->shared_from_this(), msg = req.release()] ()
		{
			std::unique_ptr<ws::Message> message(msg);
			this->AddToSendQueue(message);
		});
	}


	void Session::OnReceiveLine(std::istream& readStream, size_t size)
	{
		asio::error_code code;
		if(this->mHttpRequest == nullptr)
		{
			this->mHttpRequest = new http::Request();
		}
		this->OnReceiveMessage(readStream, size, code);
	}

	void Session::OnReceiveMessage(std::istream& readStream, size_t size, const asio::error_code& code)
	{
		if(code.value() != Asio::OK || size <= 0)
		{
			return;
		}
		int flag = 0;
		if(this->mIsHttp)
		{
			flag = this->mHttpRequest->OnRecvMessage(readStream, size);
			if(flag == tcp::read::done || flag == tcp::read::pause)
			{
				if (!this->DecodeByHttp())
				{
					this->Close(XCode::UnKnowPacket);
					return;
				}
				return;
			}
		}
		else
		{
			this->StopTimer();
			flag = this->mMessage->OnRecvMessage(readStream, size);
			if(flag == tcp::read::done)
			{
				this->OnReadBody();
				return;
			}
		}
		switch(flag)
		{
			case tcp::read::line:
				this->ReadLine();
				break;
			case tcp::read::some:
				this->ReadSome();
				break;
			case tcp::read::error:
			case tcp::read::decode_error:
				this->Close(XCode::UnKnowPacket);
				break;
			case tcp::read::big_long:
				this->Close(XCode::NetBigDataShutdown);
				break;
			default:
				this->ReadLength(flag);
				break;
		}
	}

	void Session::OnReadBody()
	{
		switch(this->mMessage->GetHeader().opcode)
		{
			case ws::OPCODE_BIN:
			case ws::OPCODE_TEXT:
			{
				if(!this->OnMessage())
				{
					this->Close(XCode::UnKnowPacket);
					return;
				}
				this->StartReceiveWebSocket();
				break;
			}
			case ws::OPCODE_CLOSE:
				this->Close(XCode::Ok);
				break;
			case ws::OPCODE_PING:
				this->OnPing();
				this->StartReceiveWebSocket();
				break;
			default:
				this->Close(XCode::UnKnowPacket);
				break;
		}
	}

	void Session::OnPing()
	{
		std::unique_ptr<ws::Message> pongMessage = std::make_unique<ws::Message>();
		{
			std::string message("pong");
			pongMessage->SetBody(ws::OPCODE_PONG, message, false);
			this->AddToSendQueue(pongMessage);
		}
	}

	bool Session::OnMessage()
	{
		const std::string & message = this->mMessage->GetMessageBody();
		std::unique_ptr<rpc::Message> request = std::make_unique<rpc::Message>();
		{
			request->SetMsg(this->mMsg);
			request->SetNet(rpc::net::ws);
			request->SetSockId(this->mSockId);
			std::istringstream buffer(message);
			if(request->OnRecvMessage(buffer, message.size()) != 0)
			{
				return false;
			}
		}
		return this->OnMessage(request);
	}

	bool Session::OnMessage(std::unique_ptr<rpc::Message>& message)
	{
		message->SetSockId(this->mSockId);
		rpc::Head & head = message->GetHead();
		for(const std::pair<std::string, std::string> & iter : this->mParameter)
		{
			head.Add(iter.first, iter.second);
		}
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(this->mMainContext, [self, this, request = message.release()]
		{
			this->mComponent->OnMessage(request, nullptr);
		});
		this->mMessage->Clear();
		return true;
	}

	void Session::Close(int code)
	{
		if(!this->mSocket->IsActive())
		{
			return;
		}

		this->StopTimer();
		this->ClearSendStream();
		this->ClearRecvStream();
		while (!this->mWaitSendMessage.empty())
		{
			this->mWaitSendMessage.pop();
		}

		std::shared_ptr<Client> self = this->shared_from_this();
		std::unique_ptr<rpc::Message> rpcMessage = std::make_unique<rpc::Message>();
		{
			rpcMessage->SetType(rpc::type::close);
			this->OnMessage(rpcMessage);
		}
		asio::post(this->mMainContext, [self, this, code, id = this->mSockId]()
		{
			this->mComponent->OnClientError(id, code);
		});
		this->mSocket->Close();
		this->mParameter.clear();
	}

	bool Session::DecodeByHttp()
	{
		if (!this->mHttpRequest->IsMethod("GET"))
		{
			return false;
		}
		const http::Head & head = this->mHttpRequest->ConstHeader();
		if(!head.IsEqual("upgrade", "websocket"))
		{
			return false;
		}
		int version = 0;
		std::string value;
		if(!head.Get("sec-websocket-key", value))
		{
			return false;
		}
		if(!head.Get("sec-websocket-version", version))
		{
			return false;
		}
		http::Response httpResponse;
		std::string shakeValue = ws::generateSecWebSocketAccept(value);
		{
			httpResponse.Header().Add("Connection", "Upgrade");
			httpResponse.Header().Add("Upgrade", "websocket");
			httpResponse.SetCode(HttpStatus::SWITCHING_PROTOCOLS);
			httpResponse.Header().Add("Sec-WebSocket-Accept", shakeValue);
			if(!this->SendSync(httpResponse))
			{
				return false;
			}
		}
		this->mIsHttp = false;
		this->StartReceiveWebSocket();
		return true;
	}

	void Session::StartReceiveWebSocket()
	{
		this->mMessage = std::make_unique<ws::Message>();
		this->ReadLength(2);
	}

	void Session::OnSendMessage(size_t size)
	{
		if(!this->mWaitSendMessage.empty())
		{
			std::unique_ptr<ws::Message> & wsMessage = this->mWaitSendMessage.front();
			{
				if(wsMessage->GetHeader().opcode == ws::OPCODE_CLOSE)
				{
					this->Close(XCode::CloseSocket);
					return;
				}
			}
			this->mWaitSendMessage.pop();
			if(!this->mWaitSendMessage.empty())
			{
				this->Write(*this->mWaitSendMessage.front());
			}
		}
	}

	void Session::AddToSendQueue(std::unique_ptr<ws::Message> & message)
	{
		this->mWaitSendMessage.emplace(std::move(message));
		if(this->mWaitSendMessage.size() == 1)
		{
			this->Write(*this->mWaitSendMessage.front());
		}
	}

	void Session::OnReadError(const Asio::Code& code)
	{
		LOG_ERROR("[{}] receive => {}", this->mSockId, code.message());
		this->Close(XCode::NetReadFailure);
	}

	void Session::OnSendMessage(const Asio::Code& code)
	{
		this->Close(XCode::NetSendFailure);
	}
}