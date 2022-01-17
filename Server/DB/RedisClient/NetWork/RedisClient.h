//
// Created by zmhy0073 on 2022/1/15.
//

#ifndef GAMEKEEPER_REDISCLIENT_H
#define GAMEKEEPER_REDISCLIENT_H
#include"SocketProxy.h"
#include"Async/TaskSource.h"
#include"RedisClient/RedisDefine.h"

namespace GameKeeper
{
    class RedisClient
    {
    public:
        RedisClient(std::shared_ptr<SocketProxy> socket);
    public:
        bool IsOpen() const { return this->mIsOpen; }
        long long GetLastOperatorTime() { return this->mLastOperatorTime;}
        std::shared_ptr<TaskSource<bool>> ConnectAsync(const std::string & ip, unsigned short port);

    private:
        void OnComplete();
        void StartReceive();
        void OnReceive(const asio::error_code & code, size_t size);
        void ConnectRedis(std::shared_ptr<TaskSource<bool>> taskSource);

    public:
        TaskSourceShared<RedisCmdResponse> WaitRedisMessageResponse();
        TaskSourceShared<RedisCmdResponse> InvokeCommand(std::shared_ptr<RedisCmdRequest> command);
    private:
        void OnDecodeHead(std::iostream & readStream);
        void OnDecodeArray(std::iostream & readStream);
        void OnClientError(const asio::error_code & code);
        void OnDecodeBinString(std::iostream & readStream);
        int OnReceiveFirstLine(char type, const std::string & lineData);
    private:
        char mReadTempBuffer[10240];
        long long mLastOperatorTime;
        std::shared_ptr<RedisCmdResponse> mResponse;
        TaskSourceShared<RedisCmdResponse> mRespTaskSource;
    private:
        int mDataSize;
        int mLineCount;
        int mDataCount;
        std::string mIp;
        unsigned short mPort;
        std::atomic_bool mIsOpen;
        NetWorkThread & mNetworkThread;
        asio::streambuf mRecvDataBuffer;
        asio::streambuf mSendDataBuffer;
        std::shared_ptr<SocketProxy> mSocket;
    };
}
#endif //GAMEKEEPER_REDISCLIENT_H
