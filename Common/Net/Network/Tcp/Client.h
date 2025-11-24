//
// Created by zmhy0073 on 2022/1/15.
//

#ifndef APP_TCPCLIENT_H
#define APP_TCPCLIENT_H
#include "Socket.h"
#include "Core/Queue/Queue.h"
#include "Rpc/Common/Message.h"
namespace tcp
{
	enum class timeout
	{
		read = 1,
		send = 2,
		connect = 3,
	};

	enum class status
	{
		none,
		send,
		read,
		close,
		connect,
	};
}

namespace tcp
{
    class IProto;
	class Client : public std::enable_shared_from_this<Client>
	{
	 public:
		explicit Client(size_t maxCount);
		explicit Client(tcp::Socket *socket, size_t maxCount);
		virtual ~Client() = default;
	public:
		void SetSocket(Socket * socket);
		inline size_t SendBufferBytes() const { return this->mRecvBuffer.capacity() * sizeof(std::streambuf::char_type); }
		inline size_t RecvBufferBytes() const { return this->mRecvBuffer.capacity() * sizeof(std::streambuf::char_type); }
		inline std::string GetAddress() const { return fmt::format("{}:{}", this->mSocket->GetIp(), this->mSocket->GetPort()); }
	protected:
		bool ReadAll(int timeout = 0);
		bool Connect(int timeout = 0);
		bool ReadSome(int timeout = 0);
		bool ReadLength(size_t size, int timeout = 0);
		void Connect(const std::string & host, const std::string & port, int timeout = 0);
		bool ReadLength(size_t size, std::function<void(const Asio::Code, size_t count)> && callback, int timeout = 0);
	protected:
		bool ReadLine(int timeout = 0);
		bool ReadLine(const std::string & delim, int timeout = 0);
		bool ReadLine(std::function<void(const Asio::Code, size_t count)> && callback, int timeout = 0);
	protected:
		void ClearBuffer();
		bool SendSync(tcp::IProto & message); //同步发送
		bool SendSync(const char * message, size_t size); //同步发送
		void Write(tcp::IProto & message, int timeout = 0);
	protected:
		void ClearSendStream();
		void ClearRecvStream();
		bool ConnectSync(Asio::Code & code);
		bool RecvSync(size_t read, size_t & size); //同步读取数据
		bool RecvSomeSync(size_t & size, unsigned int timeout = 5); //同步读取数据
		bool ConnectSync(const std::string & host, const std::string & port);
	protected:
		bool RecvLineSync(size_t & size); //同步读一行
		bool RecvLineSync(size_t & size, const std::string & delim); //同步读一行
	protected:
		void StopTimer();
		void StopUpdate();
		void StartUpdate(int timeout);
		void StartTimer(int timeout, tcp::timeout flag);
		inline tcp::status GetStatus() const { return this->mStatus; }
	protected:
		virtual void OnUpdate() { }
		virtual void OnReadError(const Asio::Code & code) = 0;
		virtual void OnConnect(const Asio::Code & code, int count) { }
        virtual void OnReceiveLine(std::istream & readStream, size_t size) {}
        virtual void OnReceiveMessage(std::istream & readStream, size_t size, const asio::error_code & code) {}
	protected:
		virtual void OnSendMessage(size_t size) { }
		virtual void OnSendMessage(const Asio::Code & code) {  };
	protected:
		int mConnectCount;
		const size_t mMaxCount;
		asio::streambuf mSendBuffer;
        asio::streambuf mRecvBuffer;
		std::unique_ptr<tcp::Socket> mSocket;
	private:
		tcp::status mStatus;
		std::unique_ptr<asio::steady_timer> mTimer;
		std::unique_ptr<asio::steady_timer> mUpdateTimer;
	};
}

#endif //APP_TCPCLIENT_H
