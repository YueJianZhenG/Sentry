﻿#include"App/App.h"
#include"Component/Timer/TimerComponent.h"
#include"Component/Coroutine/TaskComponent.h"
#include"Component/Lua/LuaScriptComponent.h"

#include<Network/Listener/TcpServerComponent.h>
#include"Global/ServiceConfig.h"
#include"Component/Redis/MainRedisComponent.h"
#include"Component/Mysql/MysqlProxyComponent.h"
#include"Component/Scene/EntityMgrComponent.h"
#include"Component/Http/HttpComponent.h"
#include"Component/Gate/GateService.h"
#include<Component/Telnet/ConsoleComponent.h>
#include<Component/Rpc/RpcHandlerComponent.h>
#include<Component/Scene/NetThreadComponent.h>
#include<Component/Rpc/RpcClientComponent.h>
#include"Component/Logic/HttpUserService.h"
#include"Component/Logic/ServiceMgrComponent.h"
#include"Component/Mysql/MysqlService.h"
#include"Component/Scene/OperatorComponent.h"
#include"Component/Scene/LoggerComponent.h"
#include"Component/Gate/GateComponent.h"
#include"Component/Gate/GateClientComponent.h"
#include"Component/Mongo/MongoService.h"
#ifdef __ENABLE_CLIENT__
#include"Component/ClientComponent.h"
using namespace Client;
#endif
#include"Component/Common/DataMgrComponent.h"
#include"Component/Gate/GateProxyComponent.h"
#include"Component/Redis/DataRedisComponent.h"
#include"Component/User/UserSyncComponent.h"
using namespace Sentry;

void RegisterComponent()
{
// rpc
    ComponentFactory::Add<RpcHandlerComponent>("RpcHandlerComponent");
	ComponentFactory::Add<ServiceMgrComponent>("ServiceMgrComponent");
// common
    ComponentFactory::Add<TaskComponent>("TaskComponent");
    ComponentFactory::Add<TimerComponent>("TimerComponent");
    ComponentFactory::Add<LoggerComponent>("LoggerComponent");
	ComponentFactory::Add<DataMgrComponent>("DataMgrComponent");
	ComponentFactory::Add<UserSyncComponent>("UserSyncComponent");
	ComponentFactory::Add<OperatorComponent>("OperatorComponent");
    ComponentFactory::Add<EntityMgrComponent>("EntityMgrComponent");
    ComponentFactory::Add<NetThreadComponent>("NetThreadComponent");

//server
	ComponentFactory::Add<ConsoleComponent>("ConsoleComponent");
    ComponentFactory::Add<TcpServerComponent>("TcpServerComponent");
    ComponentFactory::Add<RpcClientComponent>("RpcClientComponent");

// gate
    ComponentFactory::Add<GateComponent>("GateComponent");
	ComponentFactory::Add<GateProxyComponent>("GateProxyComponent");
	ComponentFactory::Add<GateClientComponent>("GateClientComponent");
// db
    ComponentFactory::Add<MainRedisComponent>("MainRedisComponent");
	ComponentFactory::Add<DataRedisComponent>("DataRedisComponent");
	ComponentFactory::Add<MysqlProxyComponent>("MysqlProxyComponent");

//http
    ComponentFactory::Add<HttpComponent>("HttpComponent");
// lua
    ComponentFactory::Add<LuaScriptComponent>("LuaScriptComponent");
#ifdef __ENABLE_CLIENT__
	ComponentFactory::Add<ClientComponent>("ClientComponent");
#endif
}

void RegisterServiceComponent()
{
    ComponentFactory::Add<GateService>("GateService");
	ComponentFactory::Add<MysqlService>("MysqlService");
	ComponentFactory::Add<MongoService>("MongoService");
	ComponentFactory::Add<HttpUserService>("HttpUserService");
}
int main(int argc, char **argv)
{
	try
    {
        RegisterComponent();
        RegisterServiceComponent();
        const std::string path(argv[1]);
        return (std::make_shared<App>(new ServerConfig(path)))->Run();
    }
    catch(std::logic_error & err)
    {
        std::cerr << err.what() << std::endl;
        return -1;
    }
}