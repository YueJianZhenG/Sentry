﻿#include"App/App.h"
#include"Component/TimerComponent.h"
#include"Component/TaskComponent.h"
#include"Component/LuaScriptComponent.h"
#include"Component/RedisDataComponent.h"
#include"Component/UnitMgrComponent.h"
#include"Component/ConsoleComponent.h"
#include"Component/NetThreadComponent.h"
#include"Component/LoggerComponent.h"
#include"Component/ProtoComponent.h"

#include"Component/InnerNetComponent.h"
#include"Component/OuterNetComponent.h"
#include"Component/OuterNetMessageComponent.h"
#include"Component/InnerNetMessageComponent.h"

#include"Component/GateHelperComponent.h"
#include"Component/ClientComponent.h"
#include"Component/RedisSubComponent.h"


#include"Component/HttpComponent.h"
#include"Component/HttpWebComponent.h"
#include"Component/LocationComponent.h"
#include"Component/TextConfigComponent.h"
#include"Component/LaunchComponent.h"
#include"Component/ForwardComponent.h"
#include"Component/SubPublishComponent.h"
#include"Component/ForwardHelperComponent.h"

#include"Service/SubPublish.h"
#include"Service/OuterService.h"
#include"Service/InnerService.h"
#include"Service/HttpWebService.h"
#include"Service/LocationService.h"
#include"Service/HttpRpcService.h"
#include"Service/UserBehavior.h"
#ifdef __ENABLE_MONGODB__
#include"Service/MongoService.h"
#include"Component/MongoDBComponent.h"
#include"Component/DataSyncComponent.h"
#include"Component/MongoHelperComponent.h"
#include"Component/MongoDataComponent.h"
#endif

#ifdef __ENABLE_MYSQL__
#include"Service/MysqlService.h"
#include"Component/MysqlDBComponent.h"
#include"Component/MysqlHelperComponent.h"
#endif

using namespace Sentry;
void RegisterComponent()
{
// rpc
    ComponentFactory::Add<InnerNetMessageComponent>("InnerNetMessageComponent");
// common
    ComponentFactory::Add<TaskComponent>("TaskComponent");
    ComponentFactory::Add<TimerComponent>("TimerComponent");
    ComponentFactory::Add<LoggerComponent>("LoggerComponent");
    ComponentFactory::Add<UnitMgrComponent>("UnitMgrComponent");
    ComponentFactory::Add<NetThreadComponent>("NetThreadComponent");
    ComponentFactory::Add<ProtoComponent>("ProtoComponent");

//server
    ComponentFactory::Add<LaunchComponent>("LaunchComponent");
    ComponentFactory::Add<TextConfigComponent>("TextConfigComponent");
	ComponentFactory::Add<ConsoleComponent>("ConsoleComponent");
    ComponentFactory::Add<InnerNetComponent>("InnerNetComponent");
	ComponentFactory::Add<LocationComponent>("LocationComponent");

    ComponentFactory::Add<ForwardComponent>("ForwardComponent");
    ComponentFactory::Add<ForwardHelperComponent>("ForwardHelperComponent");

// gate
	ComponentFactory::Add<GateHelperComponent>("GateHelperComponent");
	ComponentFactory::Add<OuterNetComponent>("OuterNetComponent");
	ComponentFactory::Add<OuterNetMessageComponent>("OuterNetMessageComponent");
// db
    ComponentFactory::Add<RedisDataComponent>("RedisDataComponent");
    ComponentFactory::Add<RedisSubComponent>("RedisSubComponent");
	ComponentFactory::Add<SubPublishComponent>("SubPublishComponent");

#ifdef __ENABLE_MONGODB__
    ComponentFactory::Add<MongoDBComponent>("MongoDBComponent");
    ComponentFactory::Add<DataSyncComponent>("DataSyncComponent");
    ComponentFactory::Add<MongoDataComponent>("MongoDataComponent");
    ComponentFactory::Add<MongoHelperComponent>("MongoHelperComponent");
#endif

#ifdef __ENABLE_MYSQL__
    ComponentFactory::Add<MysqlDBComponent>("MysqlDBComponent");
    ComponentFactory::Add<MysqlHelperComponent>("MysqlHelperComponent");
#endif
//http
    ComponentFactory::Add<HttpComponent>("HttpComponent");
    ComponentFactory::Add<HttpWebComponent>("HttpWebComponent");

    ComponentFactory::Add<HttpRpcService>("HttpRpcService");
    ComponentFactory::Add<HttpWebService>("HttpWebService");

// lua
    ComponentFactory::Add<LuaScriptComponent>("LuaScriptComponent");
    ComponentFactory::Add<Client::ClientComponent>("ClientComponent");

}

void RegisterServiceComponent()
{
    ComponentFactory::Add<SubPublish>("SubPublish");
    ComponentFactory::Add<UserBehavior>("UserBehavior");
    ComponentFactory::Add<OuterService>("OuterService");
    ComponentFactory::Add<InnerService>("InnerService");
    ComponentFactory::Add<LocationService>("LocationService");
#ifdef __ENABLE_MONGODB__
    ComponentFactory::Add<MongoService>("MongoService");
#endif

#ifdef __ENABLE_MYSQL__
    ComponentFactory::Add<MysqlService>("MysqlService");
#endif
}
int main(int argc, char **argv)
{
#ifdef __OS_WIN__
    system("chcp 65001");
#endif

    if(argc != 3)
    {
        CONSOLE_LOG_ERROR("start argc error");
        return 1;
    }
	RegisterComponent();
	RegisterServiceComponent();
	return (new App())->Run(argc, argv);
}