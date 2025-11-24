#pragma once

#include"Network/Tcp/Socket.h"
#include"Core/Queue/ThreadQueue.h"
#include"Core/Thread/AsioThread.h"
#include"Entity/Component/Component.h"
#include "Yyjson/Object/JsonObject.h"
#ifdef __ENABLE_OPEN_SSL__
#include<asio/ssl.hpp>
#endif

namespace thread
{
	struct Config : json::Object<Config>
	{
	public:
		int count = 1;
		int monitor = 15;
	};
}

namespace acs
{
	class ThreadComponent final : public Component, public IServerRecord
	{
	public:
		ThreadComponent();
		~ThreadComponent() final;
	public:
		Asio::Context & GetContext();
		Asio::Context & NewContext(const std::string & name);
	public:
		tcp::Socket * CreateSocket();
		tcp::Socket * CreateSocket(const std::string & addr);
		tcp::Socket * CreateSocket(const std::string& ip, unsigned short port);
	public:
#ifdef __ENABLE_OPEN_SSL__
		tcp::Socket * CreateSocket(Asio::ssl::Context & ssl);
#endif
	private:
		void CloseThread();
		void OnRecord(json::w::Document &document) final;
#ifndef ONLY_MAIN_THREAD
		private:
		bool Awake() final;
		void OnMonitor();
#endif
	private:
		std::mutex mLock;
#ifdef ONLY_MAIN_THREAD
		std::unique_ptr<Asio::Context> mContext;
		std::unique_ptr<Asio::ContextWork> mWork;
#else
		size_t mIndex = 0;
		thread::Config mConfig;
		std::unique_ptr<std::thread> mThread;
		std::vector<std::unique_ptr<custom::AsioThread>> mNetThreads;
		std::vector<std::unique_ptr<custom::AsioThread>> mNewThreads;
#endif
	};

}