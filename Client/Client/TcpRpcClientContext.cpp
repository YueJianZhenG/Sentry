#include"TcpRpcClientContext.h"
#include"Client/Message.h"
#include"Component/ClientComponent.h"
namespace Client
{
	TcpRpcClientContext::TcpRpcClientContext(std::shared_ptr<SocketProxy> socket, ClientComponent * component)
        : Tcp::TcpContext(socket)
	{
		this->mClientComponent = component;
        this->mState = Tcp::DecodeState::Head;
    }

	void TcpRpcClientContext::SendToServer(std::shared_ptr<Rpc::Data> message)
	{
#ifdef ONLY_MAIN_THREAD
        this->Send(message);
#else
        Asio::Context & io = this->mSocket->GetThread();
        io.post(std::bind(&TcpRpcClientContext::Send, this, message));
#endif
	}

    void TcpRpcClientContext::OnSendMessage(const Asio::Code & code, std::shared_ptr<ProtoMessage> message)
    {
        if(code)
        {
            CONSOLE_LOG_DEBUG(code.message());
            if(!this->ConnectSync())
            {
                CONSOLE_LOG_FATAL("connect server error");
                return;
            }
            this->SendFromMessageQueue();
            this->ReceiveMessage(RPC_PACK_HEAD_LEN);
        }
        else
        {
            this->PopMessage();
            this->SendFromMessageQueue();
        }
    }

    void TcpRpcClientContext::OnReceiveMessage(const asio::error_code &code, std::istream & readStream, size_t size)
    {
        if (code)
        {
#ifdef __DEBUG__
            CONSOLE_LOG_ERROR(code.message());
#endif
            return;
        }
        switch(this->mState)
        {
            case Tcp::DecodeState::Head:
            {
                int len = 0;
                this->mState = Tcp::DecodeState::Body;
                this->mMessage = std::make_shared<Rpc::Data>();
                if(!this->mMessage->ParseLen(readStream, len))
                {
                    CONSOLE_LOG_ERROR("unknow message type = "
                        << this->mMessage->GetType() << "  proto = " << this->mMessage->GetProto());
                    return;
                }
                this->ReceiveMessage(len);
            }
                break;
            case Tcp::DecodeState::Body:
            {
                this->mState = Tcp::DecodeState::Head;
                if(!this->mMessage->Parse(readStream, size))
                {
                    CONSOLE_LOG_ERROR("parse server message error");
                    return;
                }
                this->ReceiveMessage(RPC_PACK_HEAD_LEN);
                const std::string & address = this->mSocket->GetAddress();
#ifdef ONLY_MAIN_THREAD
                this->mClientComponent->OnMessage(address, std::move(this->mMessage));
#else
                Asio::Context & io = App::Get()->GetThread();
                io.post(std::bind(&ClientComponent::OnMessage,
                                  this->mClientComponent, address, std::move(this->mMessage)));
#endif
            }
                break;
		}
    }
}