//
// Created by 64658 on 2025/1/2.
//

#include "WsClient.h"
#include "XCode/XCode.h"
#include "Http/Common/HttpRequest.h"
#include "Http/Common/HttpResponse.h"
#include "Util/Tools/String.h"
#include "Proto/Bson/base64.h"
#include "Util/Crypt/sha1.h"
namespace ws
{

	std::string ComputeWebSocketAccept(const std::string& secWebSocketKey) {
		const std::string websocketGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		std::string keyWithGUID = secWebSocketKey + websocketGUID;

		// 计算 SHA-1 哈希
		std::string hash = help::Sha1::GetHash(keyWithGUID);
		return _bson::base64::encode(hash);
	}
}

namespace ws
{
	Client::Client(int id, ws::Component* component, Asio::Context& main, char msg)
			: tcp::Client(1024 * 10), mSockId(id), mComponent(component), mMainContext(main)
	{
		this->mMsg = msg;
		this->mHttpRequest = nullptr;
		this->mHttpResponse = nullptr;
	}

	void Client::OnSendMessage(size_t size)
	{
		if(this->mHttpRequest != nullptr)
		{
			this->mHttpResponse = new http::Response();
			{
				this->ReadLine();
				delete this->mHttpRequest;
				this->mHttpRequest = nullptr;
			}
		}
		else if(!this->mWaitSendMessage.empty())
		{
			this->mWaitSendMessage.pop();
			this->SendFirstMessage();
		}
	}

	void Client::OnUpdate()
	{
		std::unique_ptr<ws::Message> pingMessage = std::make_unique<ws::Message>();
		{
			std::string message("ping");
			pingMessage->SetBody(ws::OPCODE_PING, message, false);
			this->mWaitSendMessage.emplace(std::move(pingMessage));
			if(this->mWaitSendMessage.size() == 1)
			{
				this->Write(*this->mWaitSendMessage.front());
			}
		}
	}

	void Client::SendFirstMessage()
	{
		if(!this->mWaitSendMessage.empty())
		{
			this->Write(*this->mWaitSendMessage.front());
		}
	}

	void Client::OnConnect(const Asio::Code &code, int count)
	{
		if(code.value() != Asio::OK)
		{
			this->Close(XCode::NetConnectFailure);
			return;
		}
		this->mHttpRequest = new http::Request("GET");
		http::Head & head = this->mHttpRequest->Header();
		std::string randomKey = help::Str::RandomString(16);
		std::string secWebSocketAccept = ws::ComputeWebSocketAccept(randomKey);
		{
			head.Add("Connection", "Upgrade");
			head.Add("Upgrade", "websocket");
			head.Add("Sec-WebSocket-Key", secWebSocketAccept);
			head.Add("Sec-WebSocket-Version", 13);
		}
		this->ClearSendStream();
		this->Write(*this->mHttpRequest);
	}

	void Client::Close()
	{
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, self = this->shared_from_this()] ()
		{
			this->Close(XCode::CloseSocket);
		});
	}

	void Client::Close(int code)
	{
		if(this->mSockId == 0)
		{
			return;
		}

		this->StopTimer();
		this->mSocket->Close();
		this->ClearSendStream();
		this->ClearRecvStream();
		while(!this->mWaitSendMessage.empty())
		{
			this->mWaitSendMessage.pop();
		}
		if(code != XCode::CloseSocket)
		{
			auto self = this->shared_from_this();
			asio::post(this->mMainContext, [self, this, code, id = this->mSockId]()
			{
				this->mComponent->OnClientError(id, code);
			});
		}
		this->mSockId = 0;
	}

	void Client::Send(std::unique_ptr<ws::Message>& message)
	{
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, self = this->shared_from_this(), req = message.release()] ()
		{
			std::unique_ptr<ws::Message> wsMessage(req);
			this->AddToSendQueue(wsMessage);
		});

	}

	void Client::AddToSendQueue(std::unique_ptr<ws::Message>& wsMessage)
	{
		this->mWaitSendMessage.emplace(std::move(wsMessage));
		if(this->mWaitSendMessage.size() == 1)
		{
			this->Write(*this->mWaitSendMessage.front());
		}
	}

	void Client::Send(std::unique_ptr<rpc::Message>& req)
	{
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, self = this->shared_from_this(), message = req.release()] ()
		{
			this->mStream.str("");
			std::unique_ptr<ws::Message> wsMessage = std::make_unique<ws::Message>();
			{
				message->SetMsg(this->mMsg);
				message->OnSendMessage(this->mStream);
				wsMessage->SetBody(ws::OPCODE_BIN, this->mStream.str(), false);
			}
			delete message;
			this->AddToSendQueue(wsMessage);
		});
	}

	void Client::OnReadError(const Asio::Code& code)
	{
		this->Close(XCode::NetReadFailure);
	}

	void Client::OnSendMessage(const Asio::Code& code)
	{
		this->Connect(5);
	}

	void Client::OnReceiveLine(std::istream& readStream, size_t size)
	{
		asio::error_code code;
		this->OnReceiveMessage(readStream, size, code);
	}

	bool Client::OnMessage(const ws::Message& message)
	{
		const ws::Header & header = message.GetHeader();
		switch(header.opcode)
		{
			case ws::OPCODE_BIN:
			case ws::OPCODE_TEXT:
			{
				auto self = this->shared_from_this();
				const std::string & body = this->mMessage.GetMessageBody();
				std::unique_ptr<rpc::Message> request = std::make_unique<rpc::Message>();
				{
					request->SetMsg(this->mMsg);
					std::stringstream buffer(body);
					if(request->OnRecvMessage(buffer, body.size()) != 0)
					{
						return false;
					}
				}
				request->SetNet(rpc::net::ws);
				request->SetSockId(this->mSockId);
				asio::post(this->mMainContext, [self, this, req = request.release()]
				{
					this->mComponent->OnMessage(req, nullptr);
				});
				return true;
			}
			case ws::OPCODE_PING:
			{
				std::unique_ptr<ws::Message> pingMessage = std::make_unique<ws::Message>();
				{
					pingMessage->SetBody(ws::OPCODE_PONG, "ping", false);
				}
				this->AddToSendQueue(pingMessage);
				break;
			}
			case ws::OPCODE_CLOSE:
				return false;
		}
		return true;
	}

	void Client::OnReceiveMessage(std::istream& readStream, size_t size, const asio::error_code& code)
	{
		int flag = 0;
		if(this->mHttpResponse != nullptr)
		{
			flag = this->mHttpResponse->OnRecvMessage(readStream, size);
			if(flag == tcp::read::done)
			{
				if(this->mHttpResponse->Code() != HttpStatus::SWITCHING_PROTOCOLS)
				{
					this->Close(XCode::Failure);
				}
				else
				{
					this->ReadSome();
					this->SendFirstMessage();
					this->StartUpdate(ws::PING_TIME);
				}
				delete this->mHttpResponse;
				this->mHttpResponse = nullptr;
				return;
			}
		}
		else
		{
			flag = this->mMessage.OnRecvMessage(readStream, size);
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
				this->Close(XCode::UnKnowPacket);
				break;
			case tcp::read::big_long:
				this->Close(XCode::NetBigDataShutdown);
				break;
			case tcp::read::done:
			{
				if(!this->OnMessage(this->mMessage))
				{
					this->Close(XCode::UnKnowPacket);
					return;
				}
				this->mMessage.Clear();
				this->ReadLength(2);
				break;
			}
			default:
				this->ReadLength(flag);
				break;
		}
	}
}