//
// Created by yjz on 2022/4/23.
//

#ifndef _GATEAGENTCOMPONENT_H_
#define _GATEAGENTCOMPONENT_H_
#include"google/protobuf/message.h"
#include"Core/Component/Component.h"
using namespace google::protobuf;
namespace Sentry
{
	class GateComponent final : public Component, public ILuaRegister
	{
	 public:
		GateComponent() = default;
		~GateComponent() = default;
    public:
		int Send(long long userId, const std::string & func);
        int Send(long long UserId, const std::string & func, const Message & message);
	 public:
		int BroadCast(const std::string & func);
		int BroadCast(const std::string & func, const Message & message);
	 protected:
		bool LateAwake() final;
		void OnLuaRegister(Lua::ClassProxyHelper & luaRegister) final;
	 private:
		class RpcService * mGate{};
		class NodeMgrComponent* mNodeComponent{};
        class InnerNetComponent * mInnerComponent{};
	};
}

#endif //_GATEAGENTCOMPONENT_H_
