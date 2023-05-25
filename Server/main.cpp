﻿#include"Entity/Actor/App.h"
#include"Timer/Component/TimerComponent.h"
#include"Async/Component/CoroutineComponent.h"
#include"Lua/Component/LuaScriptComponent.h"
#include"Telnet/Component/ConsoleComponent.h"
#include"Server/Component/ThreadComponent.h"
#include"Log/Component/LogComponent.h"
#include"Proto/Component/ProtoComponent.h"

#include"Rpc/Component/InnerNetComponent.h"
#include"Gate/Component/OuterNetComponent.h"
#include"Rpc/Component/DispatchComponent.h"

#include"Http/Component/HttpComponent.h"
#include"Http/Component/HttpWebComponent.h"
#include"Http/Component/HttpDebugComponent.h"
#include"Entity/Component/ActorMgrComponent.h"

#include"Server/Component/TextConfigComponent.h"
#include"Cluster/Component/LaunchComponent.h"
#include"Log/Service/Log.h"
#include"Gate/Service/Gate.h"
#include"Common/Service/Node.h"
#include"Web/Service/ServerWeb.h"
#include"Common/Service/Login.h"
#include"Registry/Component/RegistryComponent.h"

#include"Component/ClientComponent.h"
#include"Kcp/Component/KcpComponent.h"

#include"Mongo/Service/MongoDB.h"
#include"Mongo/Component/MongoDBComponent.h"
#include"Mongo/Component/MongoHelperComponent.h"

#ifdef __ENABLE_MYSQL__
#include"Mysql/Service/MysqlDB.h"
#include"Mysql/Component/MysqlDBComponent.h"
#include"Mysql/Component/MysqlHelperComponent.h"
#endif

#include"Redis/Component/RedisComponent.h"
#include"Redis/Component/RedisStringComponent.h"
#include"Redis/Component/RedisLuaComponent.h"

#include"Sqlite/Component/SqliteComponent.h"

#include"WatchDog/Service/WatchDog.h"
#include"WatchDog/Component/WatchDogComponent.h"

#include"Server/Config/ServerConfig.h"
using namespace Tendo;

void RegisterComponent()
{
// common
    ComponentFactory::Add<CoroutineComponent>("CoroutineComponent");
    ComponentFactory::Add<TimerComponent>("TimerComponent");
    ComponentFactory::Add<LogComponent>("LogComponent");
    ComponentFactory::Add<ThreadComponent>("ThreadComponent");
    ComponentFactory::Add<ProtoComponent>("ProtoComponent");
	ComponentFactory::Add<ActorMgrComponent>("ActorMgrComponent");

//server
	ComponentFactory::Add<KcpComponent>("KcpComponent");
    ComponentFactory::Add<LaunchComponent>("LaunchComponent");
    ComponentFactory::Add<TextConfigComponent>("TextConfigComponent");
	ComponentFactory::Add<ConsoleComponent>("ConsoleComponent");
    ComponentFactory::Add<InnerNetComponent>("InnerNetComponent");
	ComponentFactory::Add<RegistryComponent>("RegistryComponent");
	ComponentFactory::Add<DispatchComponent>("DispatchComponent");

// gate
	ComponentFactory::Add<OuterNetComponent>("OuterNetComponent");
    ComponentFactory::Add<SqliteComponent>("SqliteComponent");
// db

    ComponentFactory::Add<RedisComponent>("RedisComponent");
    ComponentFactory::Add<RedisStringComponent>("RedisStringComponent");
    ComponentFactory::Add<RedisLuaComponent>("RedisScriptComponent");

    ComponentFactory::Add<MongoDBComponent>("MongoDBComponent");
    ComponentFactory::Add<MongoHelperComponent>("MongoHelperComponent");

#ifdef __ENABLE_MYSQL__
    ComponentFactory::Add<MysqlDBComponent>("MysqlDBComponent");
    ComponentFactory::Add<MysqlHelperComponent>("MysqlHelperComponent");
#endif
//http
    ComponentFactory::Add<HttpComponent>("HttpComponent");
    ComponentFactory::Add<HttpWebComponent>("HttpWebComponent");
    ComponentFactory::Add<HttpDebugComponent>("HttpDebugComponent");
  
    ComponentFactory::Add<ServerWeb>("ServerWeb");
    ComponentFactory::Add<WatchDogComponent>("WatchDogComponent");

// lua
    ComponentFactory::Add<LuaScriptComponent>("LuaScriptComponent");
    ComponentFactory::Add<Client::ClientComponent>("ClientComponent");
}

void RegisterAll()
{
	RegisterComponent();
	ComponentFactory::Add<Log>("Log");
	ComponentFactory::Add<Gate>("Gate");
	ComponentFactory::Add<Login>("Login");
    ComponentFactory::Add<Node>("Node");
    ComponentFactory::Add<WatchDog>("WatchDog");

    ComponentFactory::Add<MongoDB>("MongoDB");

#ifdef __ENABLE_MYSQL__
    ComponentFactory::Add<MysqlDB>("MysqlDB");
#endif
}
#include"Core/System/System.h"
int main(int argc, char **argv)
{
#ifdef __OS_WIN__
	system("chcp 65001 > nul");
#endif
	RegisterAll();
	ServerConfig config;
	System::Init(argc, argv);
	if (!config.LoadConfig(System::ConfigPath()))
	{
		CONSOLE_LOG_FATAL("load config error")
		return -1;
	}
	return std::make_shared<App>(&config)->Run();
}
