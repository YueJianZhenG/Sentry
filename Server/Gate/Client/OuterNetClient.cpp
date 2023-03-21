//
// Created by mac on 2021/11/28.
//

#include"OuterNetClient.h"
#include"Message/c2s.pb.h"
#include"Time/TimeHelper.h"
#include"App/App.h"
#ifdef __DEBUG__
#include"google/protobuf/util/json_util.h"
#endif
#include"Client/Message.h"
#include"Component/OuterNetComponent.h"

namespace Sentry
{
	OuterNetClient::OuterNetClient(std::shared_ptr<SocketProxy> socket,
                                   OuterNetComponent* component)
		: TcpContext(socket), mGateComponent(component)
	{
        this->mTimeout = 0;
        this->mState = Tcp::DecodeState::Head;
	}

	void OuterNetClient::StartReceive(int second)
	{
#ifdef ONLY_MAIN_THREAD
		this->ReceiveMessage(RPC_PACK_HEAD_LEN);
#else
        Asio::Context & t = this->mSocket->GetThread();
        t.post(std::bind(&OuterNetClient::ReceiveMessage, this, RPC_PACK_HEAD_LEN));
        if(second != 0)
        {
            this->mTimeout = second;
            t.post(std::bind(&OuterNetClient::StartTimer, this, second));
        }
#endif 
	}

	void OuterNetClient::OnTimeOut()
	{
		Asio::Context & t = App::Inst()->MainThread();
		const std::string& address = this->mSocket->GetAddress();
		t.post(std::bind(&OuterNetComponent::OnTimeout, this->mGateComponent, address));
	}

    void OuterNetClient::OnReceiveMessage(const asio::error_code &code, std::istream & readStream, size_t size)
	{
		if (code)
		{
#ifdef __NET_ERROR_LOG__
			//CONSOLE_LOG_ERROR(code.message());
			const std::string& address = this->mSocket->GetAddress();
			CONSOLE_LOG_ERROR("receive outer message error : [" << address << "]");
#endif
			this->CloseSocket(XCode::NetReceiveFailure);
			return;
		}
		switch (this->mState)
		{
			case Tcp::DecodeState::Head:
			{
				int len = 0;
				this->mState = Tcp::DecodeState::Body;
				this->mMessage = std::make_shared<Rpc::Packet>();
				if (!this->mMessage->ParseLen(readStream, len))
				{
					this->CloseSocket(XCode::UnKnowPacket);
					return;
				}
				this->ReceiveMessage(len);
			}
				break;
			case Tcp::DecodeState::Body:
			{
				this->mState = Tcp::DecodeState::Head;
				const std::string& address = this->mSocket->GetAddress();
				if (!this->mMessage->Parse(address, readStream, size))
				{
					this->CloseSocket(XCode::UnKnowPacket);
					return;
				}
				this->ReceiveMessage(RPC_PACK_HEAD_LEN);
#ifdef ONLY_MAIN_THREAD
				this->mGateComponent->OnMessage(std::move(this->mMessage));
#else
				Asio::Context& io = App::Inst()->MainThread();
				io.post(std::bind(&OuterNetComponent::OnMessage,
					this->mGateComponent, this->mMessage));
#endif
			}
				break;
		}
	}

    void OuterNetClient::CloseSocket(int code)
    {
		this->StopTimer();
		this->mSocket->Close();
        const std::string& address = this->GetAddress();
#ifdef ONLY_MAIN_THREAD
        this->mGateComponent->OnCloseSocket(address, code);
#else
        Asio::Context& mainTaskScheduler = App::Inst()->MainThread();
        mainTaskScheduler.post(std::bind(&OuterNetComponent::OnCloseSocket, this->mGateComponent, address, code));
#endif
        }

	void OuterNetClient::OnSendMessage(const asio::error_code& code, std::shared_ptr<ProtoMessage> message)
	{
		if(code)
		{
#ifdef __NET_ERROR_LOG__
			//CONSOLE_LOG_ERROR(code.message());
            const std::string& address = this->mSocket->GetAddress();
            CONSOLE_LOG_ERROR("send outer message error : [" << address << "]");
#endif
            this->PopAllMessage();
			this->CloseSocket(XCode::SendMessageFail);
            return;
		}
        this->PopMessage();
        this->SendFromMessageQueue();
	}

	void OuterNetClient::StartClose()
	{
		int code = XCode::NetActiveShutdown;
#ifdef ONLY_MAIN_THREAD
		this->CloseSocket(code);
#else
        Asio::Context & t = this->mSocket->GetThread();
        t.post(std::bind(&OuterNetClient::CloseSocket, this, code));
#endif
	}

	void OuterNetClient::SendData(std::shared_ptr<Rpc::Packet> message)
	{
#ifdef ONLY_MAIN_THREAD
		this->Write(message);
#else
        Asio::Context & t = this->mSocket->GetThread();
        t.post(std::bind(&OuterNetClient::Write, this, message));
#endif
	}
}