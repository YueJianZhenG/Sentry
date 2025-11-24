//
// Created by mac on 2021/11/28.
//

#include"OuterTcpSession.h"
#include"XCode/XCode.h"
#include"Util/Tools/TimeHelper.h"

namespace rpc
{
	OuterTcpSession::OuterTcpSession(int id, Component * component, Asio::Context & main)
		: Client(rpc::OUTER_RPC_BODY_MAX_LENGTH), mSockId(id), mMainContext(main)
	{
		this->mRecvCount = 0;
		this->mMessage = nullptr;
		this->mLastRecvTime = 0;
		this->mComponent = component;
		this->mDecodeState = tcp::Decode::None;
	}

	OuterTcpSession::~OuterTcpSession() noexcept
	{
		while(!this->mSendMessages.empty())
		{
			this->mSendMessages.pop();
		}
	}

	void OuterTcpSession::Stop()
	{
		Asio::Context & context = this->mSocket->GetContext();
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(context, [this, self] { this->CloseSocket(XCode::Ok); });
	}

	void OuterTcpSession::StartReceive(tcp::Socket * socket, int second)
	{
		this->SetSocket(socket);
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(this->mSocket->GetContext(), [this, self, second]
		{
			this->ReadLength(rpc::RPC_PACK_HEAD_LEN, second);
		});
	}

	void OuterTcpSession::OnReadError(const Asio::Code& code)
	{
		this->CloseSocket(XCode::NetReadFailure);
	}

    void OuterTcpSession::OnReceiveMessage(std::istream & readStream, size_t size, const Asio::Code & code)
	{
		if (size <= 0 || code.value() != Asio::OK)
		{
			return;
		}
		switch (this->mDecodeState)
		{
			case tcp::Decode::None:
			{
				tcp::Data::ReadHead(readStream, this->mProtoHead, true);
				if (this->mProtoHead.Len >= rpc::OUTER_RPC_BODY_MAX_LENGTH)
				{
					this->CloseSocket(XCode::NetBigDataShutdown);
					return;
				}
				this->mMessage = std::make_unique<rpc::Message>();
				{
					this->mMessage->Init(this->mProtoHead);
				}
				this->mDecodeState = tcp::Decode::MessageBody;
				if(this->mSocket->CanRecvCount() < this->mProtoHead.Len)
				{
					this->CloseSocket(XCode::NetReadFailure);
					return;
				}
				this->ReadLength(this->mProtoHead.Len);
				break;
			}
			case tcp::Decode::MessageBody:
			{
				if (this->mMessage->OnRecvMessage(readStream, size) != 0)
				{
					this->CloseSocket(XCode::UnKnowPacket);
					return;
				}
				this->mDecodeState = tcp::Decode::Done;
				break;
			}
		}
		if (this->mDecodeState != tcp::Decode::Done)
		{
			return;
		}

		switch(this->mMessage->GetType())
		{
			case rpc::type::ping:
			{
				std::unique_ptr<rpc::Message> pongMessage = std::make_unique<rpc::Message>();
				{
					pongMessage->SetType(rpc::type::pong);
					this->AddToSendQueue(pongMessage);
				}
				break;
			}
			case rpc::type::request:
			{
				if (this->mMessage->GetBody().size() >= rpc::OUTER_RPC_BODY_MAX_LENGTH)
				{
					this->CloseSocket(XCode::NetBigDataShutdown);
					return;
				}
				this->mRecvCount++;
				this->OnMessage(this->mMessage);
				break;
			}
			case rpc::type::logout:
			{
				this->CloseSocket(XCode::Ok);
				return;
			}
			default:
			{
				this->CloseSocket(XCode::UnKnowPacket);
				return;
			}
		}

		this->mDecodeState = tcp::Decode::None;
		this->mLastRecvTime = help::Time::NowSec();
		Asio::Context & context = this->mSocket->GetContext();
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(context, [this, self] { this->ReadLength(rpc::RPC_PACK_HEAD_LEN); });
	}

	void OuterTcpSession::OnMessage(std::unique_ptr<rpc::Message>& message)
	{
		message->SetSockId(this->mSockId);
#ifdef __DEBUG__
		std::string address = this->GetAddress();
		message->GetHead().Add(rpc::Header::from_addr, address);
#endif
		for(const std::pair<std::string, std::string> & iter : this->mParameter)
		{
			message->GetHead().Add(iter.first, iter.second);
		}
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(this->mMainContext, [this, self, request = message.release()]
		{
			this->mComponent->OnMessage(request, nullptr);
		});
	}

	void OuterTcpSession::SendFirstMessage()
	{
		if (!this->mSendMessages.empty())
		{
			this->Write(*this->mSendMessages.front());
		}
	}

	void OuterTcpSession::CloseSocket(int code)
	{
		if(this->mSocket->IsActive())
		{
			std::unique_ptr<rpc::Message> rpcMessage = std::make_unique<rpc::Message>();
			{
				rpcMessage->SetType(rpc::type::close);
				rpcMessage->SetSockId(this->mSockId);
				this->OnMessage(rpcMessage);
			}
			this->mParameter.clear();
			while(!this->mSendMessages.empty())
			{
				this->mSendMessages.pop();
			}
			this->StopTimer();
			this->mSocket->Close();
			std::shared_ptr<Client> self = this->shared_from_this();
			asio::post(this->mMainContext, [this, self, code]() {
				this->mComponent->OnClientError(this->mSockId, code);
			});
		}
	}

	void OuterTcpSession::OnSendMessage(size_t size)
	{
		if (!this->mSendMessages.empty())
		{
			std::unique_ptr<rpc::Message>& message = this->mSendMessages.front();
			{
				if (message->GetType() == rpc::type::close)
				{
					this->CloseSocket(XCode::CloseSocket);
					return;
				}
				this->mSendMessages.pop();
				this->SendFirstMessage();
			}
		}
	}

	void OuterTcpSession::OnSendMessage(const asio::error_code& code)
	{
		this->CloseSocket(XCode::SendMessageFail);
	}

	void OuterTcpSession::Send(std::unique_ptr<rpc::Message>& message)
	{
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(this->mSocket->GetContext(), [this, self, req = message.release()]
		{
			std::unique_ptr<rpc::Message> message(req);
			this->AddToSendQueue(message);
		});
	}

	void OuterTcpSession::AddToSendQueue(std::unique_ptr<rpc::Message> & message)
	{
		if(message->GetType() == rpc::type::parameter)
		{
			for(const std::pair<std::string, std::string> & iter : message->GetHead().GetValue())
			{
				this->mParameter.emplace_back(iter.first, iter.second);
			}
			return;
		}
		this->mSendMessages.emplace(std::move(message));
		if(this->mSendMessages.size() == 1)
		{
			this->Write(*this->mSendMessages.front());
		}
	}
}