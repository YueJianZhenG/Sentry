#pragma once
#include"Rpc/Client/Message.h"
#include"Rpc/Method/ServiceMethod.h"
#include<google/protobuf/message.h>
using namespace google::protobuf;
struct lua_State;
namespace Lua
{
	class LuaModule;
}
namespace Tendo
{

	class RpcMethodConfig;
	class LuaServiceMethod : public ServiceMethod
	{
	 public:
		LuaServiceMethod(const RpcMethodConfig * config);
	 public:
		bool IsLuaMethod() final { return true; }
		int Invoke(Msg::Packet & message) final;
	 private:
		int Call(Msg::Packet & message);
		int CallAsync(Msg::Packet & message);
	 private:
		lua_State* mLuaEnv;
        const RpcMethodConfig * mConfig;
		class LuaScriptComponent * mLuaComponent;
	};
}