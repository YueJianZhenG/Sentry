#pragma once
#include"Network/Tcp/SocketProxy.h"
#include"Entity/Component/Component.h"
namespace Tendo
{
    class TcpListenerComponent : public Component
	{
	 public:
		TcpListenerComponent();
		~TcpListenerComponent();
	 public:
        bool StopListen() const;
        bool StartListen(const char * name);
        unsigned short GetListenPort() const { return this->mListenPort;}
    protected:
        void ListenConnect();
        virtual void OnStopListen() { };
        virtual void OnListen(std::shared_ptr<Tcp::SocketProxy> socket) = 0;
    private:
		std::string mNet;
        size_t mListenCount;
        unsigned short mListenPort;
        class ThreadComponent * mThreadComponent;
        std::unique_ptr<asio::ip::tcp::acceptor> mBindAcceptor;
    };
}