#include"Socket.h"
#include"Util/Tools/String.h"
namespace tcp
{


	Socket::Socket(asio::io_service& thread)
		: mContext(thread), mIsClient(false)
	{
		this->mPort = 0;
		this->mIsClient = false;
		this->MakeNewSocket();
	}
#ifdef __ENABLE_OPEN_SSL__
	Socket::Socket(Asio::Context& io, asio::ssl::context& ssl)
		: mContext(io), mIsClient(false)
	{
		this->mPort = 0;
		this->MakeNewSocket();
#ifdef __ENABLE_OPEN_SSL__
		this->mSslSocket = std::make_unique<Asio::ssl::Socket>(*this->mSocket, ssl);
#endif
	}
#endif


	bool Socket::Init()
    {
        asio::error_code code;
        const Asio::EndPoint endPoint = this->mSocket->remote_endpoint(code);
        if (!this->mSocket->is_open() || code.value() != Asio::OK)
        {
			return false;
        }
		this->mIsClient = false;
		this->mPort = endPoint.port();
		this->mIp = endPoint.address().to_string();
		this->SetOption(tcp::OptionType::CloseLinger, true);
		this->SetOption(tcp::OptionType::ReuseAddress, true);
		return true;
    }

	size_t Socket::CanRecvCount()
	{
		Asio::Code code;
		return this->mSocket->available(code);
	}

	void Socket::MakeNewSocket()
	{
		if(this->mSocket && this->mSocket->is_open())
		{
			this->Close();
		}
		this->mSocket = std::make_unique<Asio::Socket>(this->mContext);
		{
			this->SetOption(tcp::OptionType::NoDelay, true);
			this->SetOption(tcp::OptionType::KeepAlive, true);
		}
		this->mIsActive = true;
	}

	bool Socket::Init(const std::string& address)
	{
		std::string ip;
		unsigned short port = 0;
		if(!help::Str::SplitAddr(address, ip, port))
		{
			return false;
		}
		return this->Init(ip, port);
	}

    bool Socket::Init(const std::string &ip, unsigned short port)
	{
		if (ip.empty() || port == 0)
		{
			return false;
		}
		this->mIp = ip;
		this->mPort = port;
		this->mIsClient = true;
		return true;
	}

	bool Socket::SetOption(tcp::OptionType type, bool val)
	{
		Asio::Code code;
		switch (type)
		{
			case tcp::OptionType::NoDelay:
				code = this->mSocket->set_option(asio::ip::tcp::no_delay(val), code);
				break;
			case tcp::OptionType::KeepAlive:
				code = this->mSocket->set_option(asio::ip::tcp::socket::keep_alive(val), code); //保持连接
				break;
			case tcp::OptionType::CloseLinger:
				code = this->mSocket->set_option(asio::socket_base::linger(val, 2), code); //立即关闭
				break;
			case tcp::OptionType::ReuseAddress:
				code = this->mSocket->set_option(asio::ip::tcp::socket::reuse_address(val), code); //重用地址
				break;
		}
		return code.value() == Asio::OK;
	}


	void Socket::Close()
	{
		Asio::Code code;
		this->mIsActive = false;
		code = this->mSocket->cancel(code);
		code = this->mSocket->shutdown(asio::socket_base::shutdown_both, code);
		code = this->mSocket->close(code);
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSslSocket != nullptr)
		{
			code = this->mSslSocket->shutdown(code);
		}
#endif
	}

	void Socket::Destroy()
	{
		this->Close();
		delete this;
	}
}
