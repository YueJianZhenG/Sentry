﻿
#include"InnerNetTcpClient.h"

#include"XCode/XCode.h"
#include"Entity/Actor/App.h"
#include"Rpc/Client/Rpc.h"
#include"Util/Guid/Guid.h"
#include"Core/System/System.h"
namespace Tendo
{

	InnerNetTcpClient::InnerNetTcpClient(IRpc<Msg::Packet>* component, std::shared_ptr<Tcp::SocketProxy> socket)
		: TcpContext(std::move(socket), 1024 * 1024), mIsClient(false), mComponent(component)
	{
        this->mState = Tcp::DecodeState::Head;
	}

	InnerNetTcpClient::InnerNetTcpClient(IRpc<Msg::Packet> * component,
                                   std::shared_ptr<Tcp::SocketProxy> socket, AuthInfo info)
		: TcpContext(std::move(socket), 1024 * 1024), mIsClient(true), mAuthInfo(std::move(info)), mComponent(component)
    {
        this->mState = Tcp::DecodeState::Head;
    }

	void InnerNetTcpClient::StartClose()
	{
#ifdef ONLY_MAIN_THREAD
		this->CloseSocket(XCode::NetActiveShutdown);
#else
        Asio::Context & t = this->mSocket->GetThread();
		std::shared_ptr<TcpContext> self = this->shared_from_this();
		t.post([this, self] { CloseSocket(XCode::NetActiveShutdown); });
#endif
	}

    void InnerNetTcpClient::Send(const std::shared_ptr<Msg::Packet>& message)
    {
#ifdef ONLY_MAIN_THREAD
			this->Write(message);
#else
			asio::io_service& t = this->mSocket->GetThread();
			t.post([this, message] { this->Write(message); });
#endif
    }

    void InnerNetTcpClient::OnSendMessage(const asio::error_code& code, std::shared_ptr<ProtoMessage> message)
    {
#ifdef __DEBUG__
        if (code && this->mConnectCount > 0)
        {
            const std::string& address = this->mSocket->GetAddress();
            CONSOLE_LOG_ERROR("send inner message error : [" << address << "]");
        }
#endif
        if (code && this->mIsClient)
        {
            this->Connect();
            const std::string& address = this->mSocket->GetAddress();
            CONSOLE_LOG_INFO("start connect server [" << address << "]");
        }
        else
        {
            this->PopMessage();
            this->SendFromMessageQueue();
        }
    }

    void InnerNetTcpClient::Update()
    {
        if (Helper::Time::NowSecTime() - this->GetLastOperTime() >= 30) // 30s没动静
        {

        }
    }

    void InnerNetTcpClient::CloseSocket(int code)
    {
        const std::string& address = this->mSocket->GetAddress();
        while (std::shared_ptr<ProtoMessage> data = this->PopMessage())
        {
            std::shared_ptr<Msg::Packet> packet = std::static_pointer_cast<Msg::Packet>(data);
            {
#ifdef ONLY_MAIN_THREAD
                this->mComponent->OnSendFailure(address, std::move(packet));
#else
                asio::io_service& io = App::Inst()->MainThread();
				std::shared_ptr<TcpContext> self = this->shared_from_this();
				io.post([this, address, packet, self] { this->mComponent->OnSendFailure(address, packet); });
#endif
            }
        }
        if (code == XCode::NetActiveShutdown) //主动关闭不需要通知回主线
        {
            this->mSocket->Close();
            return;
        }
#ifdef ONLY_MAIN_THREAD
        this->mComponent->OnCloseSocket(address, code);
#else
        asio::io_service& taskScheduler = App::Inst()->MainThread();
		std::shared_ptr<TcpContext> self = this->shared_from_this();
		taskScheduler.post([this, address, code, self] { this->mComponent->OnCloseSocket(address, code); });
#endif
    }

	void InnerNetTcpClient::OnReceiveMessage(const asio::error_code &code, std::istream & readStream, size_t size)
    {
        if (code)
        {
            //CONSOLE_LOG_ERROR(code.message());
#ifdef __DEBUG__
            const std::string& address = this->mSocket->GetAddress();
            CONSOLE_LOG_ERROR("receive inner message error : [" << address << "] " << code.message());
#endif
            this->CloseSocket(XCode::NetWorkError);
            return;
        }
        switch(this->mState)
        {
            case Tcp::DecodeState::Head:
            {
                int len = 0;
                Asio::Code code;
                this->mState = Tcp::DecodeState::Body;
                this->mMessage = std::make_shared<Msg::Packet>();
                if(!this->mMessage->ParseLen(readStream, len))
                {
                    this->CloseSocket(XCode::UnKnowPacket);
                    return;
                }
                //size_t count = this->mSocket->GetSocket().available(code);
                //assert(count >= len);
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
                this->mComponent->OnMessage(std::move(this->mMessage));
#else
                asio::io_service & io = App::Inst()->MainThread();
				std::shared_ptr<TcpContext> self = this->shared_from_this();
				io.post([this, self, capture0 = std::move(this->mMessage)] { this->mComponent->OnMessage(capture0); });
#endif
            }
                break;
        }
    }

	void InnerNetTcpClient::StartReceive()
	{
#ifdef ONLY_MAIN_THREAD
		this->ReceiveMessage(RPC_PACK_HEAD_LEN);
#else
        Asio::Context & t = this->mSocket->GetThread();
		std::shared_ptr<TcpContext> self = this->shared_from_this();
        t.post([this, self] { this->ReceiveMessage(RPC_PACK_HEAD_LEN); });
#endif
#ifdef __DEBUG__
        const std::string& address = this->mSocket->GetAddress();
        CONSOLE_LOG_INFO("start receive message from [" << address << "]");
#endif // __DEBUG__

	}

	void InnerNetTcpClient::OnConnect(const asio::error_code& error, int count)
	{
        if (this->mTimer != nullptr)
        {
            asio::error_code code;
            this->mTimer->cancel(code);
            this->mTimer = nullptr;
        }
		if(error)
		{
            if (this->mConnectCount >= 5)
            {
                this->CloseSocket(XCode::NetConnectFailure);
                return;
            }
#ifdef __DEBUG__
            const std::string& address = this->mSocket->GetAddress();
            CONSOLE_LOG_ERROR("connect inner error : [" << address << "]");
#endif
			this->mSocket->Close();
			Asio::Context & context = this->mSocket->GetThread();
			std::shared_ptr<TcpContext> self = this->shared_from_this();
			this->mTimer = std::make_unique<asio::steady_timer>(context, std::chrono::seconds(5));
			this->mTimer->async_wait([this, self](const asio::error_code & code){ this->Connect(); });
			return;
		}
    
		asio::io_service & io = App::Inst()->MainThread();
		const std::string & address = this->mSocket->GetAddress();
#ifdef __DEBUG__     
        CONSOLE_LOG_INFO("connect server [" << address << "] successful");
#endif
		std::shared_ptr<TcpContext> self = this->shared_from_this();
		io.post([this, address, self] { this->mComponent->OnConnectSuccessful(address); });

        std::shared_ptr<Msg::Packet> authMessage
            = std::make_shared<Msg::Packet>();
        {
            Msg::Head& head = authMessage->GetHead();
            {
                authMessage->SetType(Msg::Type::Auth);
                head.Add("name", this->mAuthInfo.ServerName);
                head.Add("user", this->mAuthInfo.UserName);
                head.Add("passwd", this->mAuthInfo.PassWord);
                head.Add("rpc", this->mAuthInfo.RpcAddress);
            }
        }
        if(this->SendSync(authMessage) > 0)
        {
            this->SendFromMessageQueue();
            this->ReceiveMessage(RPC_PACK_HEAD_LEN);
        }
        else
        {
            this->CloseSocket(XCode::NetWorkError);
        }
    }
}