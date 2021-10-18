﻿#include <Core/App.h>
#include <Scene/ActionComponent.h>
#include <Scene/LuaScriptComponent.h>
#include <Timer/TimerComponent.h>

#include <Coroutine/CoroutineComponent.h>
#include <Scene/TcpServerComponent.h>
#include <Scene/MysqlComponent.h>
#include <Scene/TcpNetSessionComponent.h>
#include <Scene/GatewayComponent.h>
#include <Scene/RedisComponent.h>
#include <Scene/MysqlProxyComponent.h>
#include <Http/HttpClientComponent.h>

#include <Service/LuaServiceMgrComponent.h>
#include <Service/ServiceMgrComponent.h>
#include <Service/ServiceNodeComponent.h>
#include <Scene/TaskPoolComponent.h>
#include <Scene/ProtocolComponent.h>
#include <Service/AccountService.h>
#include <Service/ClusterService.h>
#include <Service/CenterService.h>
#include <Scene/GatewayComponent.h>
#include <Service/GatewayService.h>


#include <Service/MysqlService.h>
using namespace Sentry;
int main(int argc, char **argv)
{
	__register_component__(GatewayComponent);
	__register_component__(TimerComponent);
	__register_component__(ServiceMgrComponent);
	__register_component__(RedisComponent);
	__register_component__(MysqlComponent);
	__register_component__(ActionComponent);
	__register_component__(LuaScriptComponent);
	__register_component__(ProtocolComponent);
    __register_component__(MysqlProxyComponent);
    __register_component__(GatewayComponent);
    __register_component__(HttpClientComponent);
	
	__register_component__(TaskPoolComponent);
	__register_component__(CoroutineComponent);
	__register_component__(ServiceNodeComponent);
	__register_component__(TcpServerComponent);
	__register_component__(TcpNetSessionComponent);

	__register_component__(LuaServiceMgrComponent);
	
	__register_component__(MysqlService);
	__register_component__(AccountService);
	__register_component__(CenterService);
	__register_component__(ClusterService);
    __register_component__(GatewayService);
	
    std::string serverName = argc == 3 ? argv[1] : "server";
    std::string configPath = argc == 3 ? argv[2] : "./Config/";

    App app(serverName, configPath);

    return app.Run();
}