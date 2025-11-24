#pragma once
#include<mutex>
#include"Asio.h"
#include"Log/Common/CommonLogDef.h"

#ifdef __ENABLE_OPEN_SSL__
#include"asio/ssl.hpp"
#endif

#ifdef __SHARE_PTR_COUNTER__
#include "Core/Memory/MemoryObject.h"
#endif
namespace tcp
{
	 enum class OptionType
	 {
		 KeepAlive,
		 NoDelay,	//发包不延迟
		 CloseLinger,
		 ReuseAddress,
	 };

	class Socket final
#ifdef __SHARE_PTR_COUNTER__
			: public memory::Object<Socket>
#endif
	{
	 public:
		explicit Socket(Asio::Context & io);
#ifdef __ENABLE_OPEN_SSL__
		explicit Socket(Asio::Context & io, asio::ssl::context & ssl);
#endif
#ifdef __SHARE_PTR_COUNTER__
		~Socket() final = default;
#else
		~Socket() { }
#endif

	 public:
		bool Init();
		void MakeNewSocket();
		size_t CanRecvCount();
		bool Init(const std::string & address);
		bool Init(const std::string & ip, unsigned short port);
		inline Asio::Socket & Get() { return *this->mSocket; }
	public:
		bool SetOption(OptionType type, bool val);
		inline Asio::Context & GetContext() { return this->mContext; }
#ifdef __ENABLE_OPEN_SSL__
		inline bool IsOpenSsl() const { return this->mSslSocket != nullptr; }
		inline Asio::ssl::Socket & SslSocket() { return *this->mSslSocket; }
#endif
    public:
        void Close();
		void Destroy();
		inline std::string & GetIp() { return this->mIp;}
		unsigned short GetPort() const { return this->mPort;}
		inline bool IsClient() const { return this->mIsClient; }
		inline bool IsActive() const { return this->mIsActive; }
	private:
		bool mIsActive;
		bool mIsClient;
		std::string mIp;
		unsigned short mPort;
		Asio::Context & mContext;
		std::unique_ptr<Asio::Socket> mSocket;
#ifdef __ENABLE_OPEN_SSL__
		std::unique_ptr<Asio::ssl::Socket> mSslSocket;
#endif
    };
}