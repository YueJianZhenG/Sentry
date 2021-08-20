﻿#include<Core/Applocation.h>
#include<Object/ReflectHelper.h>
#include<Manager/ScriptManager.h>
#include<Manager/NetSessionManager.h>
#include<Manager/ActionManager.h>
#include<Manager/ServiceManager.h>
#include<Manager/ServiceNodeManager.h>
#include<Coroutine/CoroutineManager.h>
#include<Manager/NetProxyManager.h>
#include <Timer/TimerManager.h>
#include <Manager/ClientManager.h>

#include <Manager/ProtocolManager.h>

using namespace Sentry;
using namespace Client;

int main(int argc, char ** argv)
{
	ReflectHelper<Manager>::Register<TimerManager>("TimerManager");
	ReflectHelper<Manager>::Register<ClientManager>("ClientManager");
	ReflectHelper<Manager>::Register<ScriptManager>("ScriptManager");
	ReflectHelper<Manager>::Register<ActionManager>("ActionManager");
	ReflectHelper<Manager>::Register<ServiceManager>("ServiceManager");
	ReflectHelper<Manager>::Register<NetProxyManager>("NetProxyManager");
	ReflectHelper<Manager>::Register<CoroutineManager>("CoroutineManager");
	ReflectHelper<Manager>::Register<NetSessionManager>("NetSessionManager");
	ReflectHelper<Manager>::Register<ServiceNodeManager>("ServiceNodeManager");
	ReflectHelper<Manager>::Register<ProtocolManager>("ProtocolManager");

	


	Applocation app("Client", "./Config/ClientConfig.json");

	return app.Run();
}