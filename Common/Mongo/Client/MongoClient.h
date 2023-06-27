//
// Created by mac on 2022/5/18.
//

#ifndef SERVER_MONGOCLIENT_H
#define SERVER_MONGOCLIENT_H
#include"MongoProto.h"
#include"Mongo/Config/MongoConfig.h"
#include"Network/Tcp/TcpContext.h"
#include"Network/Tcp/SocketProxy.h"
#include"Entity/Component/IComponent.h"

namespace Tendo
{
	class MongoDBComponent;
}

namespace Mongo
{
	class TcpMongoClient : public Tcp::TcpContext
	{
	 public:
		TcpMongoClient(std::shared_ptr<Tcp::SocketProxy> socket,
				Tendo::IRpc<CommandResponse> * component, const MongoConfig& config);
		TcpMongoClient(std::shared_ptr<Tcp::SocketProxy> socket, const MongoConfig & config);
	public:
        void Stop();
		bool SyncAuth();
		std::shared_ptr<CommandRequest> MakePing();
		void SendMongoCommand(const std::shared_ptr<CommandRequest>& request);
		std::shared_ptr<CommandResponse> SyncSendMongoCommand(std::shared_ptr<CommandRequest> request);
	private:
		bool StartAuthBySha1();
        bool Auth(const std::string & user, const std::string & db, const std::string & pwd);
		void OnReceiveMessage(const asio::error_code &code, std::istream & is, size_t) final;
		void OnSendMessage(const asio::error_code &code, std::shared_ptr<Tcp::ProtoMessage> message) final;
	private:
        std::string mAddress;
		const MongoConfig mConfig;
		asio::streambuf streamBuffer;
		Tendo::IRpc<CommandResponse> * mComponent;
        std::shared_ptr<CommandResponse> mMongoResponse;
	};
}


#endif //SERVER_MONGOCLIENT_H
