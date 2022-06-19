//
// Created by mac on 2022/5/18.
//

#include "MongoClient.h"

namespace Mongo
{
	MongoClientContext::MongoClientContext(std::shared_ptr<SocketProxy> scoket)
		: Tcp::TcpContext(scoket)
	{
		this->mWriteLock = std::make_shared<CoroutineLock>();
		this->mConnectLock = std::make_shared<CoroutineLock>();
	}

	void MongoClientContext::OnConnect(const asio::error_code& error, int count)
	{
		if(error)
		{
#ifdef __DEBUG__
			CONSOLE_LOG_ERROR(error.message());
#endif
			this->mConnectTask.SetResult(false);
			return;
		}
		this->mConnectTask.SetResult(true);
	}

	void MongoClientContext::OnSendMessage(const asio::error_code& code, std::shared_ptr<ProtoMessage> message)
	{
		if (code)
		{
#ifdef __DEBUG__
			CONSOLE_LOG_ERROR(code.message());
#endif
			this->mWriteTask.SetResult(false);
			return;
		}
        this->ReceiveMessage(sizeof(MongoHead));
        this->mWriteTask.SetResult(true);
	}

	bool MongoClientContext::StartConnect()
	{
		AutoCoroutineLock lock(this->mConnectLock);

		this->mConnectTask.Clear();
#ifdef ONLY_MAIN_THREAD
		this->Connect();
#else
		this->mNetworkThread.Invoke(&MongoClientContext::Connect, this);
#endif
		return this->mConnectTask.Await();
	}

	void MongoClientContext::SendMongoCommand(std::shared_ptr<Tcp::ProtoMessage> request)
	{
		AutoCoroutineLock lock(this->mWriteLock);
#ifdef ONLY_MAIN_THREAD
		this->Send(request);
#else
		this->mNetworkThread.Invoke(&MongoClientContext::Send, this, request);
#endif
		this->mWriteTask.Clear();
		if(!this->mWriteTask.Await())
		{
			LOG_ERROR("send mongo command error");
			return;
		}
		LOG_DEBUG("send mongo command successful");
	}

}