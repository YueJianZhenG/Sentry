#pragma once

#include<memory>
#include"Entity/Component/Component.h"
#include"Rpc/Method/MethodRegister.h"
namespace rpc
{
	class Message;
};
namespace Lua
{
	class LuaModule;
}

namespace acs
{
	class ServiceMethod;
	class RpcMethodConfig;
	class RpcService : public Component
	{
	public:
		RpcService();
	protected:
		bool LateAwake() final;
		virtual bool OnInit() = 0; //注册rpc回调

	public:
		const std::string& GetServer() const { return this->mCluster; }
		int Invoke(const RpcMethodConfig * config, std::unique_ptr<rpc::Message> & message);
	private:
		int CallLua(const RpcMethodConfig * config, rpc::Message & message);
		int WriterToLua(const RpcMethodConfig * config, rpc::Message & message);
		int AwaitCallLua(const RpcMethodConfig * config, rpc::Message & message);
	protected:
		std::string mCluster;
		Lua::LuaModule * mLuaModule;
		class ProtoComponent * mProto;
		ServiceMethodRegister mServiceRegister;
	};
#define BIND_RPC_METHOD(func) LOG_CHECK_RET_FALSE(this->mServiceRegister.Bind(GET_FUNC_NAME(#func), &func, rpc::Header::id ))
}