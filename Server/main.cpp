

#ifdef __ENABLE_MI_MALLOC__

#include "mimalloc-override.h"

#endif

#ifdef __OS_WIN__

#include "Log/Common/Rang.h"

#ifdef __ENABLE_MI_MALLOC__

#include "mimalloc-new-delete.h"

#endif
#endif

#include "Entity/Actor/App.h"
#include "Timer/Component/TimerComponent.h"
#include "Async/Component/CoroutineComponent.h"
#include "Lua/Component/LuaComponent.h"

#include "Server/Component/ThreadComponent.h"
#include "Proto/Component/ProtoComponent.h"

#include "Rpc/Component/InnerTcpComponent.h"
#include "Rpc/Component/OuterTcpComponent.h"
#include "Rpc/Component/DispatchComponent.h"

#include "Http/Component/HttpComponent.h"
#include "Http/Component/HttpWebComponent.h"
#include "Node/Component/NodeComponent.h"

#include "Server/Component/ConfigComponent.h"
#include "Cluster/Component/LaunchComponent.h"
#include "Log/Service/Log.h"
#include "Gate/Service/GateSystem.h"
#include "Common/Service/NodeSystem.h"
#include "Common/Service/LoginSystem.h"
#include "Gate/Component/GateComponent.h"

#include "Mongo/Component/MongoDBComponent.h"

#include "Redis/Component/RedisSubComponent.h"

#include "WX/Component/WeChatComponent.h"

#include "Client/Component/TcpClientComponent.h"
#include "Web/Service/LogMgr.h"
#include "Http/Service/ResourceMgr.h"

#include "Record/Component/RecordComponent.h"
#include "Quick/Component/QuickComponent.h"

#include "Redis/Component/DelayMQComponent.h"
#include "WX/Service/WeChat.h"
#include "Http/Component/NotifyComponent.h"

#include "WX/Component/WXComplaintComponent.h"

#include "Watch/Service/Watch.h"
#include "Watch/Component/WatchComponent.h"

#include "Udp/Component/OuterUdpComponent.h"
#include "Kcp/Component/OuterKcpComponent.h"

#include "WebSocket/Component/InnerWsComponent.h"
#include "WebSocket/Component/OuterWsComponent.h"

#include "Mysql/Component/MysqlDBComponent.h"

#include "Core/System/System.h"
#include "Redis/Component/RedisComponent.h"
#include "Sqlite/Component/SqliteComponent.h"
#include "Log/Component/LoggerComponent.h"
#include "Sqlite/Service/SqliteProxy.h"

#include "Router/Component/RouterComponent.h"

#include "Example/Service/ChatSystem.h"


#include "Upload/Service/FileUpload.h"

#include "AliCloud/Component/AliOssComponent.h"
#include "AliCloud/Component/AliSmsComponent.h"
#include "WX/Component/WXNoticeComponent.h"

#include "Quick/Service/QuickSDK.h"

#include "PubSub/Service/PubSubSystem.h"
#include "PubSub/Component/PubSubComponent.h"

#include "Client/Component/WsClientComponent.h"
#include "Web/Service/Admin.h"
#include "Pgsql/Component/PgsqlDBComponent.h"
#include "Pgsql/Component/PgsqlProxyComponent.h"
#include "Mongo/Service/MongoBackup.h"
#include "Telnet/Component/TelnetComponent.h"
#include "Common/Component/PlayerComponent.h"

#include "Registry/Service/RegistryService.h"
#include "Mongo/Service/MongoReadProxy.h"
#include "Mongo/Service/MongoWriteProxy.h"
#include "Mysql/Service/MysqlReadProxy.h"
#include "Mysql/Service/MysqlWriteProxy.h"
#include "Pgsql/Service/PgsqlReadProxy.h"
#include "Pgsql/Service/PgsqlWriteProxy.h"
#include "Mysql/Service/MysqlBackup.h"
#include "AliCloud/Service/AliOss.h"
#include "MeiliSearch/Component/MeiliSearchComponent.h"

#include "Client/Component/KcpClientComponent.h"
#include "Event/Component/EventProxyComponent.h"
#include "Event/Component/LuaEventComponent.h"
#include "Pay7xf/Component/Pay7xfComponent.h"
#include "Pay7xf/Service/Pay7xfService.h"
using namespace acs;


AUTO_REGISTER_COMPONENT(TimerComponent);
AUTO_REGISTER_COMPONENT(ProtoComponent);
AUTO_REGISTER_COMPONENT(ThreadComponent);
AUTO_REGISTER_COMPONENT(NodeComponent);
AUTO_REGISTER_COMPONENT(CoroutineComponent);

AUTO_REGISTER_COMPONENT(LaunchComponent);
AUTO_REGISTER_COMPONENT(RouterComponent);
AUTO_REGISTER_COMPONENT(InnerTcpComponent);
AUTO_REGISTER_COMPONENT(DispatchComponent);
AUTO_REGISTER_COMPONENT(ConfigComponent);

AUTO_REGISTER_COMPONENT(GateComponent);
AUTO_REGISTER_COMPONENT(OuterTcpComponent);

AUTO_REGISTER_COMPONENT(RedisSubComponent);
AUTO_REGISTER_COMPONENT(RedisComponent);
AUTO_REGISTER_COMPONENT(SqliteComponent);
AUTO_REGISTER_COMPONENT(MongoDBComponent);

AUTO_REGISTER_COMPONENT(MysqlDBComponent);

AUTO_REGISTER_COMPONENT(LuaComponent);
AUTO_REGISTER_COMPONENT(HttpComponent);
AUTO_REGISTER_COMPONENT(LoggerComponent);
AUTO_REGISTER_COMPONENT(HttpWebComponent);
AUTO_REGISTER_COMPONENT(TcpClientComponent);
AUTO_REGISTER_COMPONENT(ListenerComponent);

AUTO_REGISTER_COMPONENT(RecordComponent);

AUTO_REGISTER_COMPONENT(DelayMQComponent);
AUTO_REGISTER_COMPONENT(WXNoticeComponent);

AUTO_REGISTER_COMPONENT(NotifyComponent);

AUTO_REGISTER_COMPONENT(WatchComponent);
AUTO_REGISTER_COMPONENT(OuterUdpComponent);
AUTO_REGISTER_COMPONENT(OuterKcpComponent);
AUTO_REGISTER_COMPONENT(QuickComponent);
AUTO_REGISTER_COMPONENT(InnerWsComponent);
AUTO_REGISTER_COMPONENT(OuterWsComponent);
AUTO_REGISTER_COMPONENT(WsClientComponent);
AUTO_REGISTER_COMPONENT(KcpClientComponent);
#ifdef __ENABLE_OPEN_SSL__
AUTO_REGISTER_COMPONENT(WeChatComponent);
AUTO_REGISTER_COMPONENT(WXComplaintComponent);
#endif
AUTO_REGISTER_COMPONENT(AliOssComponent);
AUTO_REGISTER_COMPONENT(AliSmsComponent);
AUTO_REGISTER_COMPONENT(PgsqlDBComponent);
AUTO_REGISTER_COMPONENT(PgsqlProxyComponent);

AUTO_REGISTER_COMPONENT(TelnetComponent);

AUTO_REGISTER_COMPONENT(PlayerComponent);
AUTO_REGISTER_COMPONENT(Pay7xfComponent);

AUTO_REGISTER_COMPONENT(PubSubComponent);
AUTO_REGISTER_COMPONENT(MeiliSearchComponent);
AUTO_REGISTER_COMPONENT(LuaEventComponent);
AUTO_REGISTER_COMPONENT(EventProxyComponent);


AUTO_REGISTER_COMPONENT(Log);
AUTO_REGISTER_COMPONENT(Admin);
AUTO_REGISTER_COMPONENT(GateSystem);
AUTO_REGISTER_COMPONENT(NodeSystem);
AUTO_REGISTER_COMPONENT(LoginSystem);
AUTO_REGISTER_COMPONENT(MongoBackup);

#ifdef __ENABLE_OPEN_SSL__
AUTO_REGISTER_COMPONENT(WeChat);
#endif
AUTO_REGISTER_COMPONENT(ChatSystem);
AUTO_REGISTER_COMPONENT(LogMgr);
AUTO_REGISTER_COMPONENT(ResourceMgr);

AUTO_REGISTER_COMPONENT(AliOss);
AUTO_REGISTER_COMPONENT(FileUpload);

AUTO_REGISTER_COMPONENT(QuickSDK);

AUTO_REGISTER_COMPONENT(Watch);
AUTO_REGISTER_COMPONENT(PubSubSystem);

AUTO_REGISTER_COMPONENT(RegistryService);

AUTO_REGISTER_COMPONENT(MongoReadProxy);
AUTO_REGISTER_COMPONENT(MongoWriteProxy);

AUTO_REGISTER_COMPONENT(MysqlBackup);
AUTO_REGISTER_COMPONENT(MysqlReadProxy);
AUTO_REGISTER_COMPONENT(MysqlWriteProxy);

AUTO_REGISTER_COMPONENT(PgsqlReadProxy);
AUTO_REGISTER_COMPONENT(PgsqlWriteProxy);

AUTO_REGISTER_COMPONENT(SqliteProxy);
AUTO_REGISTER_COMPONENT(Pay7xfService);

int main(int argc, char** argv)
{
#if defined(__ENABLE_MI_MALLOC__) && defined(__DEBUG__)
	mi_version();
	//mi_option_enable(mi_option_show_stats);
	//mi_option_enable(mi_option_verbose);
#endif

#ifdef __OS_WIN__
	//SetConsoleOutputCP(CP_UTF8);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(hOut != INVALID_HANDLE_VALUE)
	{
		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof(CONSOLE_FONT_INFOEX);
		cfi.nFont = 4;
		wcscpy_s(cfi.FaceName, L"Consolas");
		SetCurrentConsoleFontEx(hOut, false, &cfi);
	}
	setWinTermMode(rang::winTerm::Auto);
#endif

	int id = 0;
	std::string cmd, name;
	os::System::Init(argc, argv);
	os::System::GetAppEnv("id", id);
	os::System::GetAppEnv("name", name);
	if (!os::System::GetAppEnv("cmd", cmd))
	{
		return (new App(id, name))->Run();
	}
	return (new App(id, name))->Run(cmd);
}
