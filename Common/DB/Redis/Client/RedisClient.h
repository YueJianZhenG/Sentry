//
// Created by leyi on 2023/7/27.
//

#ifndef APP_REDISCLIENT_H
#define APP_REDISCLIENT_H

#include"RedisDefine.h"
#include"Network/Tcp/Client.h"
#include"Redis/Config/RedisConfig.h"
#include"Entity/Component/IComponent.h"

namespace redis
{
	class Client final : public tcp::Client
	{
	public:
		typedef acs::IRpc<Request, Response> Component;
		Client(int id, Config  config, Component * component, Asio::Context &);
	public:
		void StartReceive();
		bool Start(tcp::Socket * socket);
		void Send(std::unique_ptr<Request>& command);
		std::unique_ptr<Response> Sync(std::unique_ptr<Request>& command);
		inline const std::string & GetVersion() const { return this->mVersion; }
	protected:
		void OnConnect(const Asio::Code & code, int count) final;
		std::unique_ptr<Response> ReadResponse(const std::unique_ptr<Request>& command);
	private:
		bool Info(json::r::Document & document);
		bool OnMessage(std::istream &readStream, size_t size, Element & element);
	private:
		bool Auth(bool connect);
		void OnSendMessage(size_t size) final;
		void OnReadError(const Asio::Code &code) final;
		void OnSendMessage(const Asio::Code &code) final;
		void OnReceiveLine(std::istream &readStream, size_t size) final;
	private:
		int mClientId;
		std::string mVersion;
		redis::Config mConfig;
		Component * mComponent;
		Asio::Context & mMainContext;
		std::unique_ptr<redis::Request> mRequest;
		std::unique_ptr<redis::Response> mResponse;
	};
}


#endif //APP_REDISCLIENT_H
