#pragma once
#include<unordered_set>
#include "ITcpComponent.h"
#include"Core/Thread/AsioThread.h"
#include"Entity/Component/Component.h"
#ifdef ONLY_MAIN_THREAD
#include"Core/Queue/Queue.h"
#else
#include"Core/Queue/DoubleBufferQueue.h"
#endif

#ifdef __ENABLE_OPEN_SSL__
#include "Util/Ssl/SslCert.h"
#endif

namespace acs
{
	class ListenerComponent final : public Component, public INetListen
#ifdef __ENABLE_OPEN_SSL__
			, public ISecondUpdate
#endif
	{
	public:
		explicit ListenerComponent();
		~ListenerComponent() override;
	public:
		bool StopListen() final;
		bool StartListen(const ListenConfig & config) final;
	private:
		void Accept();
		tcp::Socket* CreateSocket();
		void OnAcceptSocket(tcp::Socket* sock);
#ifdef __ENABLE_OPEN_SSL__
		void OnSecondUpdate(int tick) noexcept final;
#endif
	private:
		ListenConfig mConfig;
#ifdef __ENABLE_OPEN_SSL__
		Asio::ssl::Context mSslCtx;
		help::ssl::CertInfo mCertInfo;
#endif
		Asio::Executor mExecutor;
		class ITcpListen * mTcpListen;
		std::queue<tcp::Socket*> mSocketPool;
		class ThreadComponent* mThreadComponent;
		std::unique_ptr<Asio::Acceptor> mAcceptor;
		std::unordered_set<std::string> mBlackList; //黑名单
	};
}