﻿#include <Core/Applocation.h>
#include <Object/ObjectRegistry.h>
#include <Manager/ActionManager.h>
#include <Manager/ScriptManager.h>
#include <Timer/TimerManager.h>

#include <Coroutine/CoroutineManager.h>
#include <Manager/ListenerManager.h>
#include <Manager/MysqlManager.h>
#include <Manager/NetProxyManager.h>
#include <Manager/NetSessionManager.h>
#include <Manager/ProxyManager.h>
#include <Manager/RedisManager.h>
#include <Manager/ServiceManager.h>
#include <Manager/ServiceNodeManager.h>
#include <Manager/ThreadTaskManager.h>
#include <Manager/ProtocolManager.h>
#include <Manager/LoginService.h>
#include <Service/ClusterService.h>
#include <Service/ServiceRegistry.h>

#include <Service/MysqlProxy.h>
using namespace Sentry;
using namespace Sentry;

int main(int argc, char **argv)
{

    ObjectRegistry<Manager>::Register<ProxyManager>("ProxyManager");
    ObjectRegistry<Manager>::Register<RedisManager>("RedisManager");
    ObjectRegistry<Manager>::Register<MysqlManager>("MysqlManager");
    ObjectRegistry<Manager>::Register<TimerManager>("TimerManager");
    ObjectRegistry<Manager>::Register<ActionManager>("ActionManager");
    ObjectRegistry<Manager>::Register<ScriptManager>("ScriptManager");
    ObjectRegistry<Manager>::Register<ServiceManager>("ServiceManager");
    ObjectRegistry<Manager>::Register<ProtocolManager>("ProtocolManager");


    ObjectRegistry<Manager>::Register<ListenerManager>("ListenerManager");
    ObjectRegistry<Manager>::Register<CoroutineManager>("CoroutineManager");
    ObjectRegistry<Manager>::Register<ServiceNodeManager>("ServiceNodeManager");
    ObjectRegistry<Manager>::Register<NetProxyManager>("NetProxyManager");
    ObjectRegistry<Manager>::Register<NetSessionManager>("NetSessionManager");
    ObjectRegistry<Manager>::Register<ThreadTaskManager>("ThreadTaskManager");


    ObjectRegistry<LocalService>::Register<MysqlProxy>("MysqlProxy");
    ObjectRegistry<LocalService>::Register<LoginService>("LoginService");
    ObjectRegistry<LocalService>::Register<ClusterService>("ClusterService");
    ObjectRegistry<LocalService>::Register<ServiceRegistry>("ServiceRegistry");

    std::string serverName = argc == 3 ? argv[1] : "Server";
    std::string configPath = argc == 3 ? argv[2] : "./Config/ServerConfig.json";

    Applocation app(serverName, configPath);

    return app.Run();
}