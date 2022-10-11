//
// Created by mac on 2022/4/6.
//

#ifndef SERVER_LOCALSERVICECOMPONENT_H
#define SERVER_LOCALSERVICECOMPONENT_H
#include"Service.h"
#include"Method/MethodRegister.h"
#include"Component/LuaScriptComponent.h"

namespace Sentry
{
	class LocalService : public Service
	{
	public:
		LocalService();
	public:
		bool Start() final;
		bool Close() final;
        int GetWaitMessageCount() const final { return this->mWaitCount; }
		bool IsStartService() { return this->mMethodRegister != nullptr; }
		XCode Invoke(const std::string &func, std::shared_ptr<Rpc::Data> message) final;
    protected:
        virtual bool OnClose() = 0;
        virtual bool OnStart() = 0;
        void WaitAllMessageComplete() final;
        ServiceMethodRegister & GetMethodRegistry() { return *this->mMethodRegister; }
	private:
        int mWaitCount;
        bool mIsHandlerMessage;
		std::shared_ptr<ServiceMethodRegister> mMethodRegister;
	};
    extern std::string GET_FUNC_NAME(std::string fullName);
#define BIND_COMMON_RPC_METHOD(func) this->GetMethodRegistry().Bind(GET_FUNC_NAME(#func), &func);
#define BIND_ADDRESS_RPC_METHOD(func) this->GetMethodRegistry().BindAddress(GET_FUNC_NAME(#func), &func);
}
#endif //SERVER_LOCALSERVICECOMPONENT_H
