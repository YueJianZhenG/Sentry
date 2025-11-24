//
// Created by leyi on 2023/7/27.
//

#include"RedisClient.h"
#include "Util/Tools/Math.h"
#include "Entity/Actor/App.h"
#include "Core/Thread/ThreadSync.h"
#include "XCode/XCode.h"

namespace redis
{
	Client::Client(int id, Config  config, Component* com, Asio::Context& io)
			: tcp::Client(1024 * 1024), mClientId(id), mConfig(std::move(config)), mComponent(com), mMainContext(io)
	{

	}

	void Client::StartReceive()
	{
#ifdef ONLY_MAIN_THREAD
		this->ReadLine();
#else
		auto self = this->shared_from_this();
		Asio::Context& context = this->mSocket->GetContext();
		asio::post(context, [this, self]() { this->ReadLine(); });
#endif
	}

	bool Client::Start(tcp::Socket * socket)
	{
#ifdef ONLY_MAIN_THREAD
		if(socket == nullptr)
		{
			this->Connect(5);
			return true;
		}
		this->SetSocket(socket);
		if(this->Auth(true))
		{
			json::r::Document jsonDocument;
			if(this->Info(jsonDocument))
			{
				jsonDocument.Get("Server", "redis_version", this->mVersion);
			}
			return true;
		}
		return false;
#else
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		if(socket == nullptr)
		{
			Asio::Context & context = this->mSocket->GetContext();
			asio::post(context, [self, this]() {
				this->mConnectCount = 0;
				this->Connect(5);
			});
			return true;
		}
		this->SetSocket(socket);
		custom::ThreadSync<bool> threadSync;
		Asio::Socket& sock = this->mSocket->Get();
		Asio::Context & context = this->mSocket->GetContext();
		asio::post(context, [&threadSync, self, this, &sock]
		{
			bool result = this->Auth(true);
			if(result)
			{
				json::r::Document jsonDocument;
				if(this->Info(jsonDocument))
				{
					jsonDocument.Get("Server", "redis_version", this->mVersion);
				}
			}
			threadSync.SetResult(result);
		});
		return threadSync.Wait();
#endif
	}

	bool Client::Info(json::r::Document & readDocument)
	{
		std::unique_ptr<redis::Request> request = std::make_unique<redis::Request>("INFO");

		std::unique_ptr<redis::Response> response = this->ReadResponse(request);
		if (response->HasError() || !response->element.IsString())
		{
			return false;
		}
		std::string line;
		json::w::Document document;
		std::stringstream buffer(response->element.message);
		while (std::getline(buffer, line))
		{
			if (line.back() == '\r')
			{
				line.pop_back();
			}
			if (line.front() == '#')
			{
				std::string key = line.substr(2);
				std::unique_ptr<json::w::Value> jsonObject = document.AddObject(key.c_str());
				while (std::getline(buffer, line))
				{
					if (line.back() == '\r')
					{
						line.pop_back();
					}
					if (line.empty())
					{
						break;
					}
					size_t pos = line.find(':');
					std::string k = line.substr(0, pos);
					std::string v = line.substr(pos + 1);
					jsonObject->Add(k.c_str(), v);
				}
			}
		}
		return readDocument.Decode(document.JsonString());
	}

	void Client::OnConnect(const Asio::Code& code, int count)
	{
		if(code.value() != Asio::OK)
		{
			LOG_ERROR("client:{} connect => {}", this->mClientId, count);
			if(count < this->mConfig.conn_count)
			{
				this->Connect(5);
				return;
			}
		}
		else if(this->Auth(false))
		{
			LOG_DEBUG("client:{} connect ok", this->mClientId);
			if (this->mRequest != nullptr)
			{
				this->Write(*this->mRequest);
			}
			return;
		}
#ifdef ONLY_MAIN_THREAD
		if(this->mRequest != nullptr)
		{
			this->mComponent->OnSendFailure(this->mClientId, this->mRequest.release());
		}
		this->mComponent->OnClientError(this->mClientId, XCode::NetConnectFailure);
#else
		auto self = this->shared_from_this();
		asio::post(this->mMainContext, [this, self, id = this->mClientId]
		{
			if(this->mRequest != nullptr)
			{
				this->mComponent->OnSendFailure(id, this->mRequest.release());
			}
			this->mComponent->OnClientError(id, XCode::NetConnectFailure);
		});
#endif

	}

	bool Client::Auth(bool connect)
	{
		if (connect)
		{
			Asio::Code code;
			if (!this->ConnectSync(code))
			{
				LOG_ERROR("connect => {}", code.message())
				return false;
			}
		}
		if (!this->mConfig.password.empty())  //验证密码
		{
			std::unique_ptr<redis::Request> authCommand = redis::Request::Make("AUTH", this->mConfig.password);
			std::unique_ptr<redis::Response> response = this->ReadResponse(authCommand);
			if (response == nullptr || !response->IsOk())
			{
				this->mSocket->Close();
				//CONSOLE_LOG_ERROR("auth redis user failure {}", response->ToString());
				return false;
			}
		}

		if(this->mConfig.db > 0)
		{
			auto request = redis::Request::Make("SELECT", this->mConfig.db);
			std::unique_ptr<redis::Response> response = this->ReadResponse(request);
			if (response == nullptr || !response->IsOk())
			{
				//CONSOLE_LOG_ERROR("select db:{}", this->mConfig.db);
				return false;
			}
		}

		if(this->mRequest == nullptr)
		{
			std::shared_ptr<tcp::Client> self = this->shared_from_this();
			asio::post(this->mMainContext, [self, this, id = this->mClientId]()
			{
				this->mComponent->OnConnectOK(id);
			});
		}
		return true;
	}

	void Client::Send(std::unique_ptr<Request>& command)
	{
#ifdef ONLY_MAIN_THREAD
		this->mRequest = std::move(command);
		this->Write(*this->mRequest);
#else
		Asio::Context& context = this->mSocket->GetContext();
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(context, [this, self, req = command.release()]
		{
			this->mRequest.reset(req);
			this->Write(*this->mRequest);
		});
#endif

	}

	void Client::OnSendMessage(size_t size)
	{
		this->ReadLine();
	}

	void Client::OnSendMessage(const Asio::Code& code)
	{
		this->Connect(5);
		LOG_ERROR("[{}] send => {}", this->mClientId, code.message());
	}

	void Client::OnReadError(const Asio::Code& code)
	{
		this->Connect(5);
		LOG_ERROR("[{}] read => {}", this->mClientId, code.message());
	}

	bool Client::OnMessage(std::istream& readStream, size_t size, redis::Element& element)
	{
		if(size < 3)
		{
			return false;
		}
		element.type = (char)readStream.get();
		if(!std::getline(readStream, element.message))
		{
			return false;
		}
		if(element.message.back() == '\r')
		{
			element.message.pop_back();
		}
		switch (element.type)
		{
			case redis::type::Error:
			case redis::type::String:
				return true;
			case redis::type::Number:
				return help::Math::ToNumber(element.message, element.number);
			case redis::type::BinString:
			{
				int count = 0;
				if(!help::Math::ToNumber(element.message, count))
				{
					return false;
				}
				element.message.clear();
				if(count == -1)
				{
					return true;
				}
				size_t length = 0;
				if (count > 0)
				{
					int offset = 0;
					element.message.clear();
					element.message.resize(count);
					while(offset < count)
					{
						char * buffer = (char*) element.message.data() + offset;
						length = readStream.readsome(buffer, count - offset);
						if(length == 0)
						{
							this->RecvSomeSync(length);
							continue;
						}
						offset += (int)length;
					}
				}
				if(!this->RecvSync(2, length))
				{
					return false;
				}
				readStream.ignore(2);
				break;
			}
			case redis::type::Array:
			{
				int count = 0;
				size_t length = 0;
				help::Math::ToNumber(element.message, count);
				for (int index = 0; index < count; index++)
				{
					if (!this->RecvLineSync(length))
					{
						break;
					}
					Element newElement;
					if (!this->OnMessage(readStream, length, newElement))
					{
						return false;
					}
					element.list.emplace_back(newElement);
				}
				break;
			}
			default:
				return false;
		}
		return true;
	}

	void Client::OnReceiveLine(std::istream& buffer, size_t size)
	{
//		if(this->mResponse == nullptr)
//		{
//			this->mResponse = std::make_unique<redis::Response>();
//		}
		std::unique_ptr<redis::Response> response = std::make_unique<redis::Response>();
		if(!this->OnMessage(buffer, size, response->element))
		{
			response->element.message = "decode error";
			response->element.type = redis::type::Error;
		}
		size_t count = this->mRecvBuffer.size();
		if(count > 0)
		{
			std::string str;
			str.resize(count);
			std::istream is(&this->mRecvBuffer);
			is.readsome((char*)str.data(), count);
		}
		redis::Response* resp = response.release();
		redis::Request* req = this->mRequest.release();
#ifdef ONLY_MAIN_THREAD
		this->mComponent->OnMessage(this->mClientId, req, resp);
#else
		asio::post(this->mMainContext, [this, req, resp, id = this->mClientId]
		{
			this->mComponent->OnMessage(id, req, resp);
		});
#endif
	}

	std::unique_ptr<redis::Response> Client::Sync(std::unique_ptr<redis::Request>& request)
	{
#ifdef ONLY_MAIN_THREAD
		return this->ReadResponse(request);
#else
		custom::ThreadSync<bool> threadSync;
		std::unique_ptr<redis::Response> response;
		Asio::Socket& sock = this->mSocket->Get();
		const Asio::Executor& executor = sock.get_executor();
		asio::post(executor, [this, &request, &threadSync, &response]
		{
			response = this->ReadResponse(request);
			threadSync.SetResult(true);
		});
		threadSync.Wait();
		return response;
#endif
	}

	std::unique_ptr<redis::Response> Client::ReadResponse(const std::unique_ptr<redis::Request>& request)
	{
		std::unique_ptr<redis::Response> redisResponse = std::make_unique<redis::Response>();
		if (!this->SendSync(*request))
		{
			//LOG_ERROR("sync send redis cmd fail : {}", request->ToString());
			return redisResponse;
		}
		size_t readSize = 0;
		std::istream is(&this->mRecvBuffer);
		if (!this->RecvLineSync(readSize))
		{
			return redisResponse;
		}
		this->OnMessage(is, readSize, redisResponse->element);
		return redisResponse;
	}
}