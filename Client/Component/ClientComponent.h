﻿#pragma once
#include"Network/Rpc.h"
#include"Message/c2s.pb.h"
#include"Async/TaskSource.h"
#include"Script/LocalTable.h"
#include"Component/Rpc/RpcTaskComponent.h"
using namespace Sentry;
using namespace google::protobuf;

namespace Sentry
{
    class TimerComponent;
    class HttpComponent;
	class LuaScriptComponent;
	class NetThreadComponent;
}

namespace Client
{
    class ClientTask : public Sentry::IRpcTask<c2s::rpc::response>
    {
    public:
        ClientTask(int ms);
    public:
        void OnTimeout() final;
        long long GetRpcId() { return this->mTaskId; }
        void OnResponse(std::shared_ptr<c2s::rpc::response> response);
        std::shared_ptr<c2s::rpc::response> Await() { return this->mTask.Await(); }
    private:
        long long mTaskId;
        TaskSource<std::shared_ptr<c2s::rpc::response>> mTask;
    };
}

namespace Client
{
    class TcpRpcClientContext;

    class ClientComponent : public RpcTaskComponent<c2s::rpc::response>,
            public ILuaRegister,  public IRpc<Tcp::BinMessage>
    {
    public:
        ClientComponent();

        ~ClientComponent() final = default;

    public:
        void OnRequest(std::shared_ptr<c2s::rpc::call> t1);
		bool StartConnect(const std::string & ip, unsigned short port);
		std::shared_ptr<c2s::rpc::response> Call(std::shared_ptr<c2s::rpc::request> request);
        void OnMessage(const std::string &address, std::shared_ptr<Tcp::BinMessage> message) final;
    protected:

        bool LateAwake() final;
		void OnTimeout(long long rpcId);
        void OnAddTask(RpcTask task) final;
        void StartClose(const std::string &address) final;
        void OnLuaRegister(Lua::ClassProxyHelper &luaRegister) final;
        void OnCloseSocket(const std::string &address, XCode code) final;
    private:
        unsigned short mPort;
        TimerComponent *mTimerComponent;
		LuaScriptComponent * mLuaComponent;
        std::shared_ptr<TcpRpcClientContext> mTcpClient;
    };
}