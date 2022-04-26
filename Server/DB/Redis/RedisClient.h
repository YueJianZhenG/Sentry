//
// Created by zmhy0073 on 2022/1/15.
//

#ifndef GAMEKEEPER_REDISCLIENT_H
#define GAMEKEEPER_REDISCLIENT_H
#include"Network/SocketProxy.h"
#include"Async/TaskSource.h"
#include"DB/Redis/RedisDefine.h"

namespace Sentry
{
    class RedisClient : std::enable_shared_from_this<RedisClient>
    {
    public:
        RedisClient(std::shared_ptr<SocketProxy> socket, RedisConfig & config);
    public:
		bool StartConnect();
        bool IsOpen() const { return this->mIsOpen; }
		bool LoadLuaScript(const std::string & path, std::string & key);
		long long GetLastOperatorTime() { return this->mLastOperatorTime;}
        //const std::string & GetName() { return this->mSocket->GetName(); }
    private:
        void OnComplete();
        void StartReceive();
        void OnReceive(const asio::error_code & code, size_t size);
        void ConnectRedis(std::shared_ptr<TaskSource<bool>> taskSource);

    public:
        TaskSourceShared<RedisResponse> WaitRedisMessage();
        TaskSourceShared<RedisResponse> InvokeCommand(std::shared_ptr<RedisRequest> command);
    private:
        void OnDecodeHead(std::iostream & readStream);
        void OnDecodeArray(std::iostream & readStream);
        void OnClientError(const asio::error_code & code);
        void OnDecodeBinString(std::iostream & readStream);
        int OnReceiveFirstLine(char type, const std::string & lineData);
    private:
		RedisConfig & mConfig;
        char mReadTempBuffer[10240];
        long long mLastOperatorTime;
        std::shared_ptr<RedisResponse> mResponse;
        TaskSourceShared<RedisResponse> mRespTaskSource;
    private:
        int mDataSize;
        int mLineCount;
        int mDataCount;
        std::atomic_bool mIsOpen;
        IAsioThread & mNetworkThread;
        asio::streambuf mRecvDataBuffer;
        asio::streambuf mSendDataBuffer;
        std::shared_ptr<SocketProxy> mSocket;
    };
}
#endif //GAMEKEEPER_REDISCLIENT_H
