#include"SocketProxy.h"
#include<spdlog/fmt/fmt.h>
namespace Tcp
{
	SocketProxy::SocketProxy(asio::io_service& thread, const std::string & net)
		: mNet(net), mNetThread(thread)
	{
		this->mSocket = new Asio::Socket(this->mNetThread);
	}

    void SocketProxy::Init()
    {
        asio::error_code code;
        Asio::EndPoint endPoint = this->mSocket->remote_endpoint(code);
        if (this->mSocket->is_open() && !code)
        {
            this->mPort = endPoint.port();
            this->mIp = endPoint.address().to_string();
            this->mAddress = fmt::format("{0}://{1}:{2}", this->mNet, this->mIp, this->mPort);
        }
    }

    void SocketProxy::Init(const std::string &ip, unsigned short port)
    {
        if(ip.empty() || port == 0)
        {
            CONSOLE_LOG_ERROR("Inti Socket Error Address Error");
            return;
        }
		this->mIp = ip;
		this->mPort = port;
        this->mAddress = fmt::format("{0}://{1}:{2}", this->mNet, ip, port);
    }

    SocketProxy::~SocketProxy()
    {
        delete this->mSocket;
    }

	void SocketProxy::Close()
	{
		if (this->mSocket->is_open())
		{
			asio::error_code code;
			this->mSocket->close(code);
            this->mSocket->shutdown(asio::socket_base::shutdown_both, code);
		}
		asio::error_code code;
		this->mSocket->cancel(code);
	}
}
