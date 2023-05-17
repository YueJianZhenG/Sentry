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
		t.post(std::bind(&InnerNetTcpClient::CloseSocket, this, XCode::NetActiveShutdown));
#endif
	}


    bool InnerNetTcpClient::Send(const std::shared_ptr<Msg::Packet>& message, bool async)
    {
		if(async)
		{
#ifdef ONLY_MAIN_THREAD
			this->Write(message);
#else
			asio::io_service& t = this->mSocket->GetThread();
			t.post(std::bind(&InnerNetTcpClient::Write, this, message));
#endif
			return true;
		}
		else
		{
			asio::io_service& t = this->mSocket->GetThread();
			if(!App::Inst()->IsMainContext(&t))
			{
				return false;
			}
			if(this->SendSync(message) <= 0)
			{
				if(!this->ConnectSync())
				{
					return false;
				}
				return this->SendSync(message) > 0;
			}
			return true;
		}
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
//			std::shared_ptr<Msg::Packet> sendMessage = std::static_pointer_cast<Msg::Packet>(message);
//			if(sendMessage != nullptr && sendMessage->GetType() == Msg::Type::Request)
//			{
//				int rpcId = 0;
//				sendMessage->ConstHead().Get("rpc", rpcId);
//				this->mWaitResMessages.emplace(rpcId, sendMessage);
//			}

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
                io.post(std::bind(&IRpc<Msg::Packet>::OnSendFailure, this->mComponent, address, packet));
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
        taskScheduler.post(std::bind(&IRpc<Msg::Packet>::OnCloseSocket, this->mComponent, address, code));
#endif
    }

	std::shared_ptr<Msg::Packet> InnerNetTcpClient::Call(const std::shared_ptr<Msg::Packet>& message)
	{
		if(!this->Send(message, false))
		{
			return nullptr;
		}
		if(this->RecvSync(RPC_PACK_HEAD_LEN) <= 0)
		{
			return nullptr;
		}
		int len = 0;
		std::iostream readStream(&this->mRecvBuffer);
		this->mMessage = std::make_shared<Msg::Packet>();
		if(!this->mMessage->ParseLen(readStream, len))
		{
			this->CloseSocket(XCode::UnKnowPacket);
			return nullptr;
		}
		if(this->RecvSync(len) <= 0)
		{
			return nullptr;
		}
		const std::string& address = this->mSocket->GetAddress();
		if (!this->mMessage->Parse(address, readStream, len))
		{
			return nullptr;
		}
		return std::move(this->mMessage);
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
                io.post(std::bind(&IRpc<Msg::Packet>::OnMessage, this->mComponent, std::move(this->mMessage)));
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
        t.post(std::bind(&InnerNetTcpClient::ReceiveMessage, this, RPC_PACK_HEAD_LEN));
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
			this->mTimer = std::make_unique<asio::steady_timer>(context, std::chrono::seconds(5));
			this->mTimer->async_wait(std::bind(&InnerNetTcpClient::Connect, this->shared_from_this()));
			return;
		}
    
		asio::io_service & io = App::Inst()->MainThread();
		const std::string & address = this->mSocket->GetAddress();
#ifdef __DEBUG__     
        CONSOLE_LOG_INFO("connect server [" << address << "] successful");
#endif
		io.post(std::bind(&IRpc<Msg::Packet>::OnConnectSuccessful, this->mComponent, address));

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