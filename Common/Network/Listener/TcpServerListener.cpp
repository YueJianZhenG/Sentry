#include"TcpServerListener.h"
#include"App/App.h"
#include<Network/SocketProxy.h>
#include<Method/MethodProxy.h>
#include<Define/CommonLogDef.h>
#include"TcpServerComponent.h"
#include"Component/Scene/NetThreadComponent.h"
namespace Sentry
{
	TcpServerListener::TcpServerListener()
    {
		this->mCount = 0;
        this->mConfig = nullptr;
        this->mErrorCount = 0;
        this->mTcpComponent = nullptr;
        this->mBindAcceptor = nullptr;
        this->mNetComponent = nullptr;
    }

	TcpServerListener::~TcpServerListener()
	{
		asio::error_code err;
		this->mBindAcceptor->close(err);
		delete this->mBindAcceptor;
	}

    bool TcpServerListener::Init(const ListenConfig *config)
    {
        this->mConfig = config;
        this->mTcpComponent = App::Get()->GetComponent<TcpServerComponent>();
        return this->mConfig != nullptr && this->mTcpComponent != nullptr;
    }

	bool TcpServerListener::StartListen()
    {
        try
        {
            if(this->mConfig == nullptr || this->mTcpComponent == nullptr)
            {
                return false;
            }
            unsigned short port = this->mConfig->Port;
            asio::io_context & io = App::Get()->GetThread();
            AsioTcpEndPoint endPoint(asio::ip::tcp::v4(), port);
            this->mBindAcceptor = new AsioTcpAcceptor(io, endPoint);
            this->mNetComponent = App::Get()->GetComponent<NetThreadComponent>();

            this->mBindAcceptor->listen();
            this->ListenConnect();
            return true;
        }
        catch (std::system_error & err)
        {
            LOG_FATAL(fmt::format("listen {0}:{1} failure {2}",
                                  this->mConfig->Ip, this->mConfig->Port, err.what()));
            return false;
        }
    }
	void TcpServerListener::ListenConnect()
	{
        std::shared_ptr<SocketProxy> socketProxy = this->mNetComponent->CreateSocket();
		this->mBindAcceptor->async_accept(socketProxy->GetSocket(),
			[this, socketProxy](const asio::error_code & code)
		{
			if (code)
			{
				this->mErrorCount++;
                socketProxy->Close();
				CONSOLE_LOG_FATAL(this->mConfig->Name << " " << code.message() << " count = " << this->mErrorCount);
			}
			else
			{
				this->mCount++;
                socketProxy->Init();
                if(this->mTcpComponent->OnListenConnect(socketProxy))
                {
                    this->OnListen(socketProxy);
                }
            }
            this->ListenConnect();
		});
	}
}
