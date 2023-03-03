//
// Created by mac on 2022/4/6.
//

#ifndef SERVER_LOCALSERVICECOMPONENT_H
#define SERVER_LOCALSERVICECOMPONENT_H
#include"RpcService.h"
#include"Method/EventMethod.h"
#include"Method/MethodRegister.h"
#include"Component/LuaScriptComponent.h"

namespace Sentry
{
    class PhysicalService : public RpcService, public IServerRecord
	{
	public:
		PhysicalService();
	public:
		bool Start() final;
		bool Close() final;
		bool IsStartService() final { return true; }
		int GetWaitMessageCount() const final { return this->mWaitCount; }
		int Invoke(const std::string &func, std::shared_ptr<Rpc::Packet> message) final;
    protected:
        bool LoadFromLua() final;
        virtual bool OnClose() = 0;
        virtual bool OnStart() = 0;
        void WaitAllMessageComplete() final;
        void OnRecord(Json::Writer&document) final;
		ServiceMethodRegister & GetMethodRegistry() { return *this->mMethodRegister; }
	private:
        unsigned int mSumCount;
        unsigned int mWaitCount;
        bool mIsHandlerMessage;
		std::unique_ptr<ServiceMethodRegister> mMethodRegister;
	};
    extern std::string GET_FUNC_NAME(std::string fullName);
#define BIND_COMMON_RPC_METHOD(func) LOG_CHECK_RET_FALSE(this->GetMethodRegistry().Bind(GET_FUNC_NAME(#func), &func));
}
#endif //SERVER_LOCALSERVICECOMPONENT_H
