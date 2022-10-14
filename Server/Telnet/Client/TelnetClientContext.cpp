
#include"TelnetClientContext.h"
#include"String/StringHelper.h"
#include"Log/CommonLogDef.h"
#include"Component/ConsoleComponent.h"
namespace Tcp
{
	TelnetClientContext::TelnetClientContext(std::shared_ptr<SocketProxy> socketProxy, ConsoleComponent * component)
		: Tcp::TcpContext(socketProxy), mConsoleComponent(component)
	{
		this->mSocket = socketProxy;
	}

	void TelnetClientContext::StartRead()
	{
#ifdef ONLY_MAIN_THREAD
		this->ReceiveLine();
#else
		Asio::Context & netWorkThread = this->mSocket->GetThread();
		netWorkThread.post(std::bind(&TelnetClientContext::ReceiveLine, this));
#endif
	}

	void TelnetClientContext::OnReceiveLine(const Asio::Code & code, std::istream & is, size_t)
	{
		if(code)
		{
			this->CloseContext();
			CONSOLE_LOG_ERROR(code.message());
			return;
		}
		std::string lineMessage;
		std::getline(is, lineMessage);
		const std::string & address = this->GetAddress();
#ifdef ONLY_MAIN_THREAD
		this->mConsoleComponent->OnReceive(address, lineMessage);
#else
		Asio::Context & netWorkThread = App::Inst()->GetThread();
		netWorkThread.post(std::bind(&ConsoleComponent::OnReceive, this->mConsoleComponent,address , lineMessage));
#endif
	}

	void TelnetClientContext::SendProtoMessage(std::shared_ptr<TelnetProto> message)
	{
#ifdef ONLY_MAIN_THREAD
		this->Send(message);
#else
		Asio::Context &netWorkThread = this->mSocket->GetThread();
		netWorkThread.post(std::bind(&TelnetClientContext::Send, this, message));
#endif
	}

	void TelnetClientContext::OnSendMessage(const Asio::Code & code, std::shared_ptr<ProtoMessage> message)
	{
		if(code)
		{
			this->CloseContext();
			CONSOLE_LOG_ERROR(code.message());
		}
        this->PopMessage();
	}

	void TelnetClientContext::CloseContext()
	{
		this->mSocket->Close();
		const std::string & address = this->GetAddress();
#ifdef ONLY_MAIN_THREAD
		this->mConsoleComponent->OnClientError(address);
#else
		asio::io_service & netWorkThread = App::Inst()->GetThread();
		netWorkThread.post(std::bind(&ConsoleComponent::OnClientError, this->mConsoleComponent, address));
#endif
	}
}

