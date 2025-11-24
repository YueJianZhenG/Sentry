#include"InnerTcpClient.h"

#include"XCode/XCode.h"
#include"Net/Rpc/Common/Rpc.h"
#include"Core/System/System.h"
#include"Core/Queue/Queue.h"

namespace rpc
{
	InnerTcpClient::InnerTcpClient(int id, Component* component, bool client, Asio::Context & io)
			: Client(rpc::INNER_RPC_BODY_MAX_LENGTH), mSockId(id), mComponent(component),
			  mIsClient(client), mDecodeStatus(tcp::Decode::None), mMainContext(io)
	{

	}

	InnerTcpClient::~InnerTcpClient() noexcept
	{
		while (!this->mSendMessages.empty())
		{
			this->mSendMessages.pop();
		}
	}

	bool InnerTcpClient::Send(std::unique_ptr<rpc::Message> & message)
	{
		LOG_CHECK_RET_FALSE(message);
		if (message->GetType() == rpc::type::response)
		{
			assert(message->GetRpcId() > 0);
		}
#ifdef ONLY_MAIN_THREAD
		this->AddToSendQueue(message);
#else
		auto self = this->shared_from_this();
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, self, req = message.release()]
		{
			std::unique_ptr<rpc::Message> message(req);
			this->AddToSendQueue(message);
		});
#endif
		return true;
	}

	void InnerTcpClient::OnSendMessage(size_t size)
	{
		if (!this->mSendMessages.empty())
		{
			std::unique_ptr<rpc::Message> & waitMessage = this->mSendMessages.front();
			{
				if (waitMessage->GetType() == rpc::type::request && waitMessage->GetRpcId() != 0)
				{
					int rpcId = waitMessage->GetRpcId();
					this->mWaitResMessages.emplace(rpcId, std::move(waitMessage));
				}
				this->mSendMessages.pop();
			}
			if (!this->mSendMessages.empty())
			{
				this->Write(*this->mSendMessages.front());
			}
		}
	}

	void InnerTcpClient::OnConnect(const Asio::Code &code, int count)
	{
		if (code.value() == Asio::OK)
		{
			this->StopTimer();
			this->ClearSendStream();
			if (!this->mSendMessages.empty())
			{
				this->Write(*this->mSendMessages.front());
			}
			this->ReadLength(rpc::RPC_PACK_HEAD_LEN);
			this->mSocket->SetOption(tcp::OptionType::NoDelay, true);
			this->mSocket->SetOption(tcp::OptionType::KeepAlive, true);
			return;
		}
		this->CloseSocket(XCode::NetConnectFailure);
	}

	void InnerTcpClient::OnSendMessage(const Asio::Code& code)
	{
		if (code != asio::error::operation_aborted
			&& this->mIsClient && this->mSocket->IsActive())
		{
			this->Connect(5);
			return;
		}
		this->CloseSocket(XCode::NetSendFailure);
	}

	void InnerTcpClient::CloseSocket()
	{
		if(!this->mSocket->IsActive())
		{
			return;
		}
		this->mDecodeStatus = tcp::Decode::None;
		if (this->mComponent == nullptr)
		{
			this->mWaitResMessages.clear();
			while (!this->mSendMessages.empty())
			{
				this->mSendMessages.pop();
			}
			return;
		}
		auto iter = this->mWaitResMessages.begin();
		for (; iter != this->mWaitResMessages.end(); iter++) //发出去没有返回的数据
		{
			this->mSendMessages.emplace(std::move(iter->second));
		}
		this->mWaitResMessages.clear();

		auto self = this->shared_from_this();
		while (!this->mSendMessages.empty())
		{
			std::unique_ptr<rpc::Message> & message = this->mSendMessages.front();
			{
				asio::post(this->mMainContext, [this, self, req = message.release()]
				{
					this->mComponent->OnSendFailure(this->mSockId, req);
				});
				this->mSendMessages.pop();
			}
		}
		this->mSocket->Close();
	}

	void InnerTcpClient::CloseSocket(int code)
	{
		if(this->mSocket->IsActive())
		{
			this->CloseSocket();
			std::shared_ptr<Client> self = this->shared_from_this();
			asio::post(this->mMainContext, [self, this, code]()
			{
				this->mComponent->OnClientError(this->mSockId, code);
			});
		}
	}

	void InnerTcpClient::OnReadError(const Asio::Code& code)
	{
		if (code != asio::error::operation_aborted)
		{
			this->CloseSocket(XCode::NetReadFailure);
		}
	}

	bool InnerTcpClient::MakeMessage(const rpc::ProtoHead& header)
	{
		if (header.type != rpc::type::response)
		{
			this->mMessage = std::make_unique<rpc::Message>();
			return true;
		}

		int rpcId = header.rpcId;
		auto iter = this->mWaitResMessages.find(rpcId);
		if (iter == this->mWaitResMessages.end())
		{
			return false;
		}
		this->mMessage = std::move(iter->second);
		this->mMessage->SetMsg(rpc::msg::bin);
		this->mWaitResMessages.erase(iter);
		return true;
	}

	void InnerTcpClient::OnReceiveMessage(std::istream& readStream, size_t size, const Asio::Code& code)
	{
		if (size == 0 || code.value() != Asio::OK)
		{
			return;
		}
		if(this->mDecodeStatus == tcp::Decode::None)
		{
			tcp::Data::ReadHead(readStream, this->mProtoHead, true);

			if (this->mProtoHead.Len >= this->mMaxCount)
			{
				this->CloseSocket(XCode::NetBigDataShutdown);
				return;
			}
			if (!this->MakeMessage(this->mProtoHead))
			{
				this->CloseSocket(XCode::UnKnowPacket);
				return;
			}
			this->mMessage->Init(this->mProtoHead);
			this->mDecodeStatus = tcp::Decode::MessageBody;
			this->ReadLength(this->mProtoHead.Len);
			return;
		}
		else if(this->mDecodeStatus == tcp::Decode::MessageBody)
		{
			if (this->mMessage->OnRecvMessage(readStream, size) != 0)
			{
				this->CloseSocket(XCode::UnKnowPacket);
				return;
			}
		}
		do
		{
			if (this->mComponent == nullptr)
			{
				this->mMessage.reset();
				break;
			}
			switch(this->mMessage->GetType())
			{
				case rpc::type::ping:
				{
					std::unique_ptr<rpc::Message> pingMessage = std::make_unique<rpc::Message>();
					{
						pingMessage->SetType(rpc::type::pong);
						this->AddToSendQueue(pingMessage);
					}
					break;
				}
				case rpc::type::pong:
					break;
				default:
				{
					rpc::Message* request = this->mMessage.release();
					{
						this->mMessage = nullptr;
						request->SetSockId(this->mSockId);
					}
#ifdef __DEBUG__
					std::string address = this->GetAddress();
					request->GetHead().Add(rpc::Header::from_addr, address);
#endif
					asio::post(this->mMainContext, [this, request] { this->mComponent->OnMessage(request, nullptr); });
					break;
				}
			}

		} while (false);
		this->mDecodeStatus = tcp::Decode::None;
		Asio::Context& context = this->mSocket->GetContext();
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(context, [this, self]() { this->ReadLength(rpc::RPC_PACK_HEAD_LEN); });
	}

	void InnerTcpClient::AddToSendQueue(std::unique_ptr<rpc::Message> & message)
	{
		message->SetMsg(rpc::msg::bin);
		this->mSendMessages.emplace(std::move(message));
		if(this->mSendMessages.size() == 1)
		{
			this->Write(*this->mSendMessages.front());
		}
	}

	void InnerTcpClient::Close()
	{
#ifdef ONLY_MAIN_THREAD
		this->CloseSocket();
#else
		Asio::Context & context = this->mSocket->GetContext();
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::post(context, [this, self] { this->CloseSocket(); });
#endif
	}

	void InnerTcpClient::StartReceive(tcp::Socket* socket)
	{
		this->SetSocket(socket);
#ifdef ONLY_MAIN_THREAD
		this->mSocket->SetOption(tcp::OptionType::NoDelay, true);
		this->mSocket->SetOption(tcp::OptionType::KeepAlive, true);
		this->ReadLength(rpc::RPC_PACK_HEAD_LEN);
#else
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [this, self = this->shared_from_this()]
		{
			this->mSocket->SetOption(tcp::OptionType::NoDelay, true);
			this->mSocket->SetOption(tcp::OptionType::KeepAlive, true);
			this->ReadLength(rpc::RPC_PACK_HEAD_LEN);
		});
#endif
	}
}