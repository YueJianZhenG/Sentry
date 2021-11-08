﻿#include <Core/App.h>
#include <Scene/CallHandlerComponent.h>
#include <Scene/LuaScriptComponent.h>
#include <Timer/TimerComponent.h>

#include <Coroutine/CoroutineComponent.h>
#include <Network/Listener/TcpServerComponent.h>
#include <Scene/MysqlComponent.h>
#include <Network/Tcp/TcpClientComponent.h>
#include <Scene/GatewayComponent.h>
#include <Scene/RedisComponent.h>
#include <Scene/MysqlProxyComponent.h>
#include <Network/Http/HttpClientComponent.h>
#include <Network/Telnet/TelnetClientComponent.h>

#include <Service/LuaServiceMgrComponent.h>
#include <Service/ServiceMgrComponent.h>
#include <Service/ServiceNodeComponent.h>
#include <Scene/TaskPoolComponent.h>
#include <Scene/ProtocolComponent.h>
#include <Service/AccountService.h>
#include <Service/ClusterService.h>
#include <Service/CenterService.h>
#include <Service/GatewayService.h>
#include <Scene/DeamonComponent.h>
#include <Service/MysqlService.h>
#include <HttpService/HttpResourceComponent.h>
#include <Pool/StringPool.h>
#include <Service/HttpLoginService.h>
using namespace GameKeeper;

int main(int argc, char **argv)
{

    for (int index = 0; index < argc; index++)
    {
        printf("%s\n", argv[index]);
    }
    __register_component__(GatewayComponent);
    __register_component__(TimerComponent);
    __register_component__(ServiceMgrComponent);
    __register_component__(RedisComponent);
    __register_component__(MysqlComponent);
    __register_component__(CallHandlerComponent);
    __register_component__(LuaScriptComponent);
    __register_component__(ProtocolComponent);
    __register_component__(MysqlProxyComponent);
    __register_component__(GatewayComponent);
    __register_component__(HttpClientComponent);

    __register_component__(TaskPoolComponent);
    __register_component__(CoroutineComponent);
    __register_component__(ServiceNodeComponent);
    __register_component__(TcpServerComponent);
    __register_component__(TcpClientComponent);
    __register_component__(TelnetClientComponent);
    __register_component__(DeamonComponent);

    __register_component__(LuaServiceMgrComponent);

    __register_component__(MysqlService);
    __register_component__(AccountService);
    __register_component__(CenterService);
    __register_component__(ClusterService);
    __register_component__(GatewayService);
    __register_component__(HttpLoginService);
    __register_component__(HttpResourceComponent);

    App app(argc, argv);
    return app.Run();
}