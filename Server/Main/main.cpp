﻿#include"App/App.h"
#include"Component/Timer/TimerComponent.h"
#include"Component/Coroutine/TaskComponent.h"
#include"Component/Lua/LuaScriptComponent.h"

#include"Network/Listener/TcpServerComponent.h"
#include"Global/ServiceConfig.h"
#include"Component/Redis/DataRedisComponent.h"
#include"Component/Scene/UnitMgrComponent.h"
#include"Component/Http/HttpComponent.h"
#include"Component/Gate/GateService.h"
#include"Component/Telnet/ConsoleComponent.h"
#include"Component/Rpc/TcpRpcComponent.h"
#include"Component/Scene/NetThreadComponent.h"
#include"Component/Rpc/RpcServerComponent.h"
#include"Component/Logic/RedisRegistryComponent.h"
#include"Component/Scene/OperatorComponent.h"
#include"Component/Scene/LoggerComponent.h"
#include"Component/Gate/GateComponent.h"
#include"Component/Gate/RpcGateComponent.h"
#include"Component/Scene/MessageComponent.h"
#include"Component/Logic/HttpSourceService.h"
#include"Component/RpcService/LuaService.h"
#include"Component/Http/HttpWebComponent.h"
#include"Component/Mongo/MongoService.h"
#include"Component/HttpService/LocalLuaHttpService.h"
#include"Component/Mongo/MongoRpcComponent.h"
#include"Component/Mysql/MysqlRpcComponent.h"

#include"Component/Common/DataMgrComponent.h"
#include"Component/Gate/GateAgentComponent.h"
#include"Component/User/UserSyncComponent.h"
#include"Component/ClientComponent.h"

#include"Component/Gate/JsonClientComponent.h"
#include"Component/Redis/RedisSubComponent.h"
#include"Component/Http/HttpWebComponent.h"
#include"Component/Http/HttpRpcComponent.h"
#include"Component/RpcService/ServiceAgent.h"
using namespace Sentry;
void RegisterComponent()
{
// rpc
    ComponentFactory::Add<TcpRpcComponent>("TcpRpcComponent");
	ComponentFactory::Add<RedisRegistryComponent>("RedisRegistryComponent");
// common
    ComponentFactory::Add<TaskComponent>("TaskComponent");
    ComponentFactory::Add<TimerComponent>("TimerComponent");
    ComponentFactory::Add<LoggerComponent>("LoggerComponent");
	ComponentFactory::Add<DataMgrComponent>("DataMgrComponent");
	ComponentFactory::Add<UserSyncComponent>("UserSyncComponent");
	ComponentFactory::Add<OperatorComponent>("OperatorComponent");
    ComponentFactory::Add<UnitMgrComponent>("UnitMgrComponent");
    ComponentFactory::Add<NetThreadComponent>("NetThreadComponent");
    ComponentFactory::Add<MessageComponent>("MessageComponent");

//server
	ComponentFactory::Add<ConsoleComponent>("ConsoleComponent");
    ComponentFactory::Add<TcpServerComponent>("TcpServerComponent");
    ComponentFactory::Add<RpcServerComponent>("RpcServerComponent");

// gate
    ComponentFactory::Add<GateComponent>("GateComponent");
	ComponentFactory::Add<GateAgentComponent>("GateAgentComponent");
	ComponentFactory::Add<RpcGateComponent>("RpcGateComponent");
// db
    ComponentFactory::Add<MysqlRpcComponent>("MysqlRpcComponent");
	ComponentFactory::Add<MongoRpcComponent>("MongoRpcComponent");
    ComponentFactory::Add<DataRedisComponent>("DataRedisComponent");
    ComponentFactory::Add<RedisSubComponent>("RedisSubComponent");

//http
    ComponentFactory::Add<HttpComponent>("HttpComponent");
    ComponentFactory::Add<HttpWebComponent>("HttpWebComponent");
    ComponentFactory::Add<HttpRpcComponent>("HttpRpcComponent");

// lua
    ComponentFactory::Add<LuaScriptComponent>("LuaScriptComponent");

    ComponentFactory::Add<Client::ClientComponent>("ClientComponent");

    ComponentFactory::Add<JsonClientComponent>("JsonClientComponent");

}

void RegisterServiceComponent()
{
    ComponentFactory::Add<LuaService>("rpc");
    ComponentFactory::Add<ServiceAgent>("agent");
    ComponentFactory::Add<LocalLuaHttpService>("http");
    ComponentFactory::Add<GateService>("GateService");
	ComponentFactory::Add<MongoService>("MongoService");
	ComponentFactory::Add<HttpSourceService>("HttpSourceService");
}
int main(int argc, char **argv)
{
	try
    {
        RegisterComponent();
        RegisterServiceComponent();
        return (std::make_shared<App>(new ServerConfig(argc, argv)))->Run();
    }
    catch(std::logic_error & err)
    {
        std::cerr << err.what() << std::endl;
        return -1;
    }
}