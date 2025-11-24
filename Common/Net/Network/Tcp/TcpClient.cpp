//
// Created by zmhy0073 on 2022/1/15.
//
#include"Client.h"
#include"Util/Tools/String.h"
#include"Proto/Message/IProto.h"
#include "Util/Tools/Math.h"

namespace tcp
{
	Client::Client(size_t maxCount)
			: mMaxCount(maxCount)
	{
		this->mConnectCount = 0;
		this->mStatus = tcp::status::none;
	}

	Client::Client(Socket* socket, size_t maxCount)
			: mMaxCount(maxCount)
	{
		this->mConnectCount = 0;
		this->mSocket.reset(socket);
		this->mStatus = tcp::status::none;
		this->mRecvBuffer.prepare(maxCount);
		//this->mSendBuffer.prepare(maxCount);
	}

	void Client::SetSocket(Socket* socket)
	{
		this->mConnectCount = 0;
		this->ClearSendStream();
		this->ClearRecvStream();
		this->mSocket.reset(socket);
	}

	void Client::ClearBuffer()
	{
		this->ClearSendStream();
		this->ClearRecvStream();
	}

	bool Client::Connect(int timeout)
	{
		if (this->mStatus == tcp::status::connect)
		{
			return false;
		}

		Asio::Code code;
		this->ClearBuffer();
		this->mConnectCount++;
		this->mStatus = tcp::status::connect;
		unsigned short port = this->mSocket->GetPort();
		const std::string& ip = this->mSocket->GetIp();
		Asio::Address address = asio::ip::make_address(ip, code);
		if (code.value() != Asio::OK)
		{
			this->mStatus = tcp::status::none;
			this->OnConnect(code, this->mConnectCount);
			return false;
		}

		this->mSocket->MakeNewSocket();
		Asio::EndPoint endPoint(address, port);
		Asio::Socket& sock = this->mSocket->Get();
		this->StartTimer(timeout, tcp::timeout::connect);
		std::shared_ptr<Client> self = this->shared_from_this();
		sock.async_connect(endPoint, [self, timeout](const Asio::Code& code)
		{
			if (timeout > 0)
			{
				self->StopTimer();
			}
			self->mStatus = tcp::status::none;
#ifdef __ENABLE_OPEN_SSL__
			if (self->mSocket != nullptr && self->mSocket->IsOpenSsl())
			{
				self->mSocket->SslSocket().async_handshake(asio::ssl::stream_base::client,
						[self](const asio::error_code& err)
						{
							self->OnConnect(err, self->mConnectCount);
						});
				return;
			}
#endif
			if (code.value() == Asio::OK)
			{
				self->mConnectCount = 0;
			}
			self->OnConnect(code, self->mConnectCount);
		});
		return true;
	}

	void Client::Connect(const std::string& host, const std::string& port, int timeout)
	{
		if (help::Str::IsIpAddress(host))
		{
			int connectPort = 0;
			if (!help::Math::ToNumber(port, connectPort))
			{
				Asio::Code code(asio::error::address_family_not_supported);
				this->OnConnect(code, 1);
				return;
			}
			this->mSocket->Init(host, connectPort);
			this->Connect(timeout);
			return;
		}
		this->mConnectCount++;
		Asio::Socket& sock = this->mSocket->Get();
		const Asio::Executor& executor = sock.get_executor();

		Asio::Code code;
		Asio::Resolver resolver(executor);
		Asio::ResolverQuery query(host, port);
		Asio::Resolver::iterator iterator = resolver.resolve(query, code);
		if (code.value() != Asio::OK)
		{
			this->OnConnect(code, 1);
			return;
		}
		this->StartTimer(timeout, tcp::timeout::connect);
		std::shared_ptr<Client> self = this->shared_from_this();
		asio::async_connect(sock, iterator, [iterator, self, timeout]
				(const asio::error_code& code, const Asio::Resolver::iterator& iter)
		{
			if (timeout > 0)
			{
				self->StopTimer();
			}
#ifdef __ENABLE_OPEN_SSL__
			if (self->mSocket && self->mSocket->IsOpenSsl())
			{
				self->mSocket->SslSocket().async_handshake(asio::ssl::stream_base::client,
						[self](const asio::error_code& err)
						{
							//CONSOLE_LOG_FATAL("{}", err.message())
							self->OnConnect(err, self->mConnectCount);
						});
				return;
			}
#endif
			if (code.value() == Asio::OK)
			{
				self->mConnectCount = 0;
			}
			self->OnConnect(code, self->mConnectCount);
		});
	}

	bool Client::ReadAll(int timeout)
	{
		if (this->mStatus == tcp::status::read
			|| this->mStatus == tcp::status::connect)
		{
			return false;
		}
		this->mStatus = tcp::status::read;
		this->StartTimer(timeout, tcp::timeout::read);
		std::shared_ptr<Client> self = this->shared_from_this();
		auto callBack = [self, timeout](const asio::error_code& code, size_t size)
		{
			if (timeout > 0)
			{
				self->StopTimer();
			}
			self->mStatus = tcp::status::none;
			std::istream ss(&self->mRecvBuffer);
			if (code.value() == Asio::OK)
			{
				self->OnReceiveMessage(ss, size, code);
				return;
			}
			if (self->mRecvBuffer.size() > 0)
			{
				size = self->mRecvBuffer.size();
				self->OnReceiveMessage(ss, size, code);
			}
			if (code != asio::error::operation_aborted)
			{
				self->OnReadError(code);
			}
		};

#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			asio::async_read(sock, this->mRecvBuffer, asio::transfer_all(), callBack);
			return true;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		asio::async_read(sock, this->mRecvBuffer, asio::transfer_all(), callBack);
		return true;
	}

	bool Client::ReadLine(int timeout)
	{
		if (this->mStatus == tcp::status::read
			|| this->mStatus == tcp::status::connect)
		{
			return false;
		}
		this->mStatus = tcp::status::read;
		this->StartTimer(timeout, tcp::timeout::read);
		std::shared_ptr<Client> self = this->shared_from_this();
		auto callback = [self, timeout](const Asio::Code& code, size_t size)
		{
			if (timeout > 0)
			{
				self->StopTimer();
			}
			self->mStatus = tcp::status::none;
			std::istream ss(&self->mRecvBuffer);
			if (code.value() == Asio::OK)
			{
				self->OnReceiveLine(ss, size);
				return;
			}
			if (self->mRecvBuffer.size() > 0)
			{
				size = self->mRecvBuffer.size();
				self->OnReceiveLine(ss, size);
				return;
			}
			if (code != asio::error::operation_aborted)
			{
				self->OnReadError(code);
			}
		};
		const std::string delim("\r\n");
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			asio::async_read_until(sock, this->mRecvBuffer, delim, callback);
			return true;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		asio::async_read_until(sock, this->mRecvBuffer, delim, callback);
		return true;
	}

	bool Client::ReadLine(std::function<void(const Asio::Code, size_t)>&& callback, int timeout)
	{
		const static std::string delim("\r\n");
		this->StartTimer(timeout, tcp::timeout::read);
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			asio::async_read_until(sock, this->mRecvBuffer, delim, callback);
			return true;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		asio::async_read_until(sock, this->mRecvBuffer, delim, callback);
		return true;
	}

	bool Client::ReadLine(const std::string& delim, int timeout)
	{
		if (this->mStatus == tcp::status::read
			|| this->mStatus == tcp::status::connect)
		{
			return false;
		}
		this->mStatus = tcp::status::read;
		this->StartTimer(timeout, tcp::timeout::read);
		std::shared_ptr<Client> self = this->shared_from_this();
		auto callback = [self, timeout](const Asio::Code& code, size_t size)
		{
			if (timeout > 0)
			{
				self->StopTimer();
			}
			self->mStatus = tcp::status::none;
			std::istream ss(&self->mRecvBuffer);
			if (code.value() == Asio::OK)
			{
				self->OnReceiveLine(ss, size);
				return;
			}
			if (self->mRecvBuffer.size() > 0)
			{
				size = self->mRecvBuffer.size();
				self->OnReceiveLine(ss, size);
				return;
			}
			if (code != asio::error::operation_aborted)
			{
				self->OnReadError(code);
			}
		};
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			asio::async_read_until(sock, this->mRecvBuffer, delim, callback);
			return true;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		asio::async_read_until(sock, this->mRecvBuffer, delim, callback);
		return true;
	}

	bool Client::ReadSome(int timeout)
	{
		std::istream ss(&this->mRecvBuffer);
		if (this->mRecvBuffer.size() > 0)
		{
			asio::error_code code;
			size_t size = this->mRecvBuffer.size();
			this->OnReceiveMessage(ss, size, code);
			return true;
		}

		if (this->mStatus == tcp::status::read
			|| this->mStatus == tcp::status::connect)
		{
			return false;
		}
		this->mStatus = tcp::status::read;
		this->StartTimer(timeout, tcp::timeout::read);
		std::shared_ptr<Client> self = this->shared_from_this();
		auto callBack = [self, timeout](const asio::error_code& code, size_t size)
		{
			self->mStatus = tcp::status::none;
			if (timeout > 0)
			{
				self->StopTimer();
			}
			std::istream ss(&self->mRecvBuffer);
			if (code.value() == Asio::OK)
			{
				self->OnReceiveMessage(ss, size, code);
				return;
			}
			if(size > 0)
			{
				self->OnReceiveMessage(ss, size, code);
			}
			else if (self->mRecvBuffer.size() > 0)
			{
				size_t count = self->mRecvBuffer.size();
				self->OnReceiveMessage(ss, count, code);
			}
			if (code != asio::error::operation_aborted
				&& code != asio::error::eof)
			{
				self->OnReadError(code);
			}
		};
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			asio::async_read(sock, this->mRecvBuffer, asio::transfer_at_least(1), callBack);
			return true;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		asio::async_read(sock, this->mRecvBuffer, asio::transfer_at_least(1), callBack);
		return true;
	}

	bool Client::ReadLength(size_t length, int timeout)
	{
		if (length <= 0)
		{
			//CONSOLE_LOG_FATAL("length = {}", length);
			this->OnReadError(std::make_error_code(std::errc::bad_message));
			return false;
		}
		if (this->mMaxCount > 0 && length >= this->mMaxCount)
		{
			this->OnReadError(std::make_error_code(std::errc::bad_message));
			return false;
		}
		std::istream ss(&this->mRecvBuffer);
		if (this->mRecvBuffer.size() >= length)
		{
			asio::error_code code;
			this->OnReceiveMessage(ss, length, code);
			return true;
		}

		if (this->mStatus == tcp::status::read
			|| this->mStatus == tcp::status::connect)
		{
			return false;
		}
		this->mStatus = tcp::status::read;
		this->StartTimer(timeout, tcp::timeout::read);
		std::shared_ptr<Client> self = this->shared_from_this();
		auto callBack = [self, length, timeout](const asio::error_code& code, size_t size)
		{
			if (timeout > 0)
			{
				self->StopTimer();
			}
			if(self->mRecvBuffer.size() >= length)
			{
				size = length;
			}
			self->mStatus = tcp::status::none;
			std::istream ss(&self->mRecvBuffer);
			if (code.value() == Asio::OK)
			{
				self->OnReceiveMessage(ss, size, code);
				return;
			}
			if(size > 0)
			{
				self->OnReceiveMessage(ss, size, code);
			}
			if (code != asio::error::operation_aborted)
			{
				self->OnReadError(code);
			}
		};
		size_t count = length - this->mRecvBuffer.size();
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			asio::async_read(sock, this->mRecvBuffer, asio::transfer_exactly(count), callBack);
			return true;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		asio::async_read(sock, this->mRecvBuffer, asio::transfer_exactly(count), callBack);
		return true;
	}

	bool Client::ReadLength(size_t length, std::function<void(const Asio::Code, size_t)>&& callback, int timeout)
	{
		if (length <= 0)
		{
			//CONSOLE_LOG_FATAL("length = {}", length);
			callback(std::make_error_code(std::errc::bad_message), 0);
			return false;
		}
		if (this->mMaxCount > 0 && length >= this->mMaxCount)
		{
			callback(std::make_error_code(std::errc::bad_message), 0);
			return false;
		}
		std::istream ss(&this->mRecvBuffer);
		if (this->mRecvBuffer.size() >= length)
		{
			asio::error_code code;
			callback(code, length);
			return true;
		}
		this->StartTimer(timeout, tcp::timeout::read);
		size_t count = length - this->mRecvBuffer.size();
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			asio::async_read(sock, this->mRecvBuffer, asio::transfer_exactly(count), callback);
			return true;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		asio::async_read(sock, this->mRecvBuffer, asio::transfer_exactly(count), callback);
		return true;
	}

	void Client::Write(tcp::IProto& message, int timeout)
	{
		std::ostream os(&this->mSendBuffer);
		int length = message.OnSendMessage(os);
		this->StartTimer(timeout, tcp::timeout::send);
		std::shared_ptr<Client> self = this->shared_from_this();
		auto callBack = [self, length, &message, timeout](const Asio::Code& code, size_t size)
		{
			if (code.value() != Asio::OK)
			{
				self->OnSendMessage(code);
				return;
			}
			if (length > 0)
			{
				self->Write(message, timeout);
				return;
			}
			self->OnSendMessage(size);
		};

#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			asio::async_write(sock, this->mSendBuffer, callBack);
			return;
		}
#endif

		Asio::Socket& sock = this->mSocket->Get();
		asio::async_write(sock, this->mSendBuffer, callBack);
	}
}

namespace tcp
{
	void Client::ClearRecvStream()
	{
		if (this->mRecvBuffer.size() > 0)
		{
			std::iostream os(&this->mRecvBuffer);
			os.ignore((int)this->mRecvBuffer.size());
		}
	}

	bool Client::ConnectSync(Asio::Code& code)
	{
		if (!this->mSocket->IsClient())
		{
			return false;
		}
		unsigned short port = this->mSocket->GetPort();
		const std::string& ip = this->mSocket->GetIp();
		asio::ip::address address = asio::ip::make_address(ip);
		return this->mSocket->Get().connect(Asio::EndPoint(address, port), code).value() == Asio::OK;
	}

	bool Client::ConnectSync(const std::string& host, const std::string& port)
	{

		int connPort = 0;
		Asio::Code code;
		if (help::Str::IsIpAddress(host))
		{
			if (!help::Math::ToNumber(port, connPort))
			{
				return false;
			}
			Asio::Address address = asio::ip::make_address(host, code);
			if (code.value() != Asio::OK)
			{
				return false;
			}
			Asio::EndPoint endPoint(address, connPort);
			return this->mSocket->Get().connect(endPoint, code).value() == Asio::OK;
		}
		Asio::Socket& sock = this->mSocket->Get();
		const Asio::Executor& executor = sock.get_executor();
		Asio::Resolver resolver(executor);
		Asio::ResolverQuery query(host, port);
		Asio::Resolver::iterator iterator = resolver.resolve(query, code);
		if (code.value() != Asio::OK)
		{
			return false;
		}
		asio::connect(sock, iterator, code);
		if (code.value() != Asio::OK)
		{
			return false;
		}
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			return this->mSocket->SslSocket().handshake(asio::ssl::stream_base::client, code).value() == Asio::OK;
		}
#endif
		return true;
	}

	void Client::ClearSendStream()
	{
		if (this->mSendBuffer.size() > 0)
		{
			std::iostream os(&this->mSendBuffer);
			os.ignore((int)this->mSendBuffer.size());
		}
	}

	bool Client::RecvSomeSync(size_t& size, unsigned int timeout)
	{
		if (this->mRecvBuffer.size() > 0)
		{
			size = this->mRecvBuffer.size();
			return true;
		}
		if(this->mSocket->CanRecvCount() == 0)
		{
			this->mSocket->Get().wait(asio::socket_base::wait_read);
		}
		Asio::Code code;
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			size = asio::read(sock, this->mRecvBuffer, asio::transfer_at_least(1), code);
			if (code.value() != Asio::OK)
			{
				this->OnReadError(code);
				return false;
			}
			return true;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		size = asio::read(sock, this->mRecvBuffer, asio::transfer_at_least(1), code);
		if (code.value() != Asio::OK)
		{
			this->OnReadError(code);
			return false;
		}
		return true;
	}

	bool Client::RecvSync(size_t length, size_t& size)
	{
		Asio::Code code;
		do
		{
			if (this->mRecvBuffer.size() >= length)
			{
				size = length;
				return true;
			}
			size_t count = length - this->mRecvBuffer.size();
#ifdef __ENABLE_OPEN_SSL__
			if (this->mSocket->IsOpenSsl())
			{
				Asio::ssl::Socket& sock = this->mSocket->SslSocket();
				asio::read(sock, this->mRecvBuffer, asio::transfer_exactly(count), code);
				break;
			}
#endif
			Asio::Socket& sock = this->mSocket->Get();
			asio::read(sock, this->mRecvBuffer, asio::transfer_exactly(count), code);
		}
		while (false);
		if(code.value() != Asio::OK && code != asio::error::eof)
		{
			this->OnReadError(code);
			return false;
		}
		if(this->mRecvBuffer.size() < length)
		{
			return false;
		}
		size = length;
		return true;
	}

	bool Client::RecvLineSync(size_t& size)
	{
		Asio::Code code;
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			size = asio::read_until(sock, this->mRecvBuffer, "\r\n", code);
			return code.value() == Asio::OK;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		size = asio::read_until(sock, this->mRecvBuffer, "\r\n", code);
		return code.value() == Asio::OK;
	}

	bool Client::RecvLineSync(size_t& size, const std::string& delim)
	{
		Asio::Code code;
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			size = asio::read_until(sock, this->mRecvBuffer, delim, code);
			return code.value() == Asio::OK;
		}
#endif
		Asio::Socket& sock = this->mSocket->Get();
		size = asio::read_until(sock, this->mRecvBuffer, delim, code);
		return code.value() == Asio::OK;
	}

	bool Client::SendSync(tcp::IProto& message)
	{
		int length = 0;
		Asio::Code code;
		do
		{
			size_t count = 0;
			std::ostream os(&this->mSendBuffer);
			length = message.OnSendMessage(os);
#ifdef __ENABLE_OPEN_SSL__
			if (this->mSocket->IsOpenSsl())
			{
				Asio::ssl::Socket& sock = this->mSocket->SslSocket();
				count = asio::write(sock, this->mSendBuffer, code);
			}
			else
			{
				count = asio::write(this->mSocket->Get(), this->mSendBuffer, code);
			}
#else
			count = asio::write(this->mSocket->Get(), this->mSendBuffer, code);
#endif
			if (code.value() != Asio::OK || count <= 0)
			{
				return false;
			}
		} while (length > 0);
		return true;
	}

	bool Client::SendSync(const char* message, size_t size)
	{
		Asio::Code code;
#ifdef __ENABLE_OPEN_SSL__
		if (this->mSocket->IsOpenSsl())
		{
			Asio::ssl::Socket& sock = this->mSocket->SslSocket();
			asio::write(sock, asio::buffer(message, size), code);
			return code.value() == Asio::OK;
		}
		asio::write(this->mSocket->Get(), asio::buffer(message, size), code);
		return code.value() == Asio::OK;
#else
		asio::write(this->mSocket->Get(), asio::buffer(message, size), code);
		return code.value() == Asio::OK;
#endif
	}

	void Client::StopTimer()
	{
		if (this->mTimer != nullptr)
		{
			asio::error_code code;
			this->mTimer->cancel(code);
			this->mTimer.reset();
		}
	}

	void Client::StopUpdate()
	{
		if (this->mUpdateTimer != nullptr)
		{
			asio::error_code code;
			this->mUpdateTimer->cancel(code);
			this->mUpdateTimer.reset();
		}
	}

	void Client::StartUpdate(int timeout)
	{
		if (this->mUpdateTimer == nullptr)
		{
			Asio::Socket& sock = this->mSocket->Get();
			const Asio::Executor& executor = sock.get_executor();
			this->mUpdateTimer = std::make_unique<Asio::Timer>(executor);
		}
		else
		{
			Asio::Code code;
			this->mUpdateTimer->cancel(code);
		}
		std::shared_ptr<Client> self = this->shared_from_this();
		this->mUpdateTimer->expires_after(std::chrono::seconds(timeout));
		this->mUpdateTimer->async_wait([self, timeout](const Asio::Code& code)
		{
			if (code.value() == Asio::OK)
			{
				self->OnUpdate();
			}
			if (code == asio::error::operation_aborted)
			{
				return;
			}
			self->StartUpdate(timeout);
		});
	}

	void Client::StartTimer(int timeout, tcp::timeout flag)
	{
		if (timeout <= 0) return;
		if (this->mTimer == nullptr)
		{
			Asio::Socket& sock = this->mSocket->Get();
			const Asio::Executor& executor = sock.get_executor();
			this->mTimer = std::make_unique<Asio::Timer>(executor);
		}
		else
		{
			Asio::Code code;
			this->mTimer->cancel(code);
		}

		std::shared_ptr<Client> self = this->shared_from_this();
		this->mTimer->expires_after(std::chrono::seconds(timeout));
		this->mTimer->async_wait([self, timeout, flag](const Asio::Code& code)
		{
			if (code.value() == Asio::OK)
			{
				self->mStatus = tcp::status::none;
				Asio::Code code(asio::error::timed_out);
				switch (flag)
				{
					case tcp::timeout::connect:
						self->OnConnect(code, 1);
						break;
					case tcp::timeout::read:
						self->OnReadError(code);
						break;
					case tcp::timeout::send:
						self->OnSendMessage(code);
						break;
				}
			}
		});
	};
}