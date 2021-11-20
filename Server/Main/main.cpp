﻿
#include <Core/App.h>
#include <jemalloc/jemalloc.h>
#include <Scene/RpcResponseComponent.h>
#include <Scene/LuaScriptComponent.h>
#include <Timer/TimerComponent.h>

#include <Coroutine/CoroutineComponent.h>
#include <Listener/TcpServerComponent.h>
#include <Scene/MysqlComponent.h>
#include <Scene/RpcConfigComponent.h>
#include <Scene/GatewayComponent.h>
#include <Scene/RedisComponent.h>
#include <Scene/MysqlProxyComponent.h>
#include <Scene/HttpComponent.h>
#include <Telnet/TelnetClientComponent.h>

#include <Service/LuaServiceMgrComponent.h>
#include <Scene/RpcRequestComponent.h>
#include <Service/NodeProxyComponent.h>
#include <Scene/TaskPoolComponent.h>
#include <Scene/ProtoRpcComponent.h>
#include <Service/AccountService.h>
#include <Service/NodeServer.h>
#include <Service/NodeCenter.h>
#include <Service/GatewayService.h>
#include <Scene/MonitorComponent.h>
#include <Service/MysqlService.h>
#include <Http/HttpResourceComponent.h>
#include <Service/HttpLoginService.h>
#include <Http/HttpOperComponent.h>
#include "Scene/OperatorComponent.h"
using namespace GameKeeper;

int main(int argc, char **argv)
{
    //je_malloc(100);
    __register_component__(GatewayComponent);
    __register_component__(TimerComponent);
    __register_component__(RpcRequestComponent);
    __register_component__(RedisComponent);
    __register_component__(MysqlComponent);
    __register_component__(RpcResponseComponent);
    __register_component__(LuaScriptComponent);
    __register_component__(RpcConfigComponent);
    __register_component__(MysqlProxyComponent);
    __register_component__(GatewayComponent);
    __register_component__(HttpComponent);
    __register_component__(OperatorComponent);

    __register_component__(TaskPoolComponent);
    __register_component__(CoroutineComponent);
    __register_component__(NodeProxyComponent);
    __register_component__(TcpServerComponent);
    __register_component__(ProtoRpcComponent);
    __register_component__(TelnetClientComponent);
    __register_component__(MonitorComponent);

    __register_component__(LuaServiceMgrComponent);

    __register_component__(MysqlService);
    __register_component__(AccountService);
    __register_component__(NodeCenter);
    __register_component__(NodeServer);
    __register_component__(GatewayService);
    __register_component__(HttpLoginService);
    __register_component__(HttpOperComponent);
    __register_component__(HttpResourceComponent);


    App app(argc, argv);
    return app.Run();
}