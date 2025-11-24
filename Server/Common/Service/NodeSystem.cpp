//
// Created by zmhy0073 on 2022/10/8.
//

#include "NodeSystem.h"
#include "Util/Tools/Math.h"
#include "Entity/Actor/App.h"
#include "Core/System/System.h"
#include "Util/Tools/TimeHelper.h"
#include "Yyjson/Document/Document.h"
#include "Event/Base/IEvent.h"
#include "Timer/Timer/ElapsedTimer.h"
#include "Lua/Component/LuaComponent.h"
#include "Server/Component/ConfigComponent.h"
#ifdef __OS_LINUX__
#include <malloc.h>
#endif

#ifdef __SHARE_PTR_COUNTER__

#include "Rpc/Client/InnerTcpClient.h"
#include "Rpc/Client/OuterTcpSession.h"
#include "Http/Client/HttpClient.h"
#include "Http/Client/HttpSession.h"
#include "Async/Lua/LuaWaitTaskSource.h"
#include "Redis/Client/RedisDefine.h"
#include "Http/Common/HttpRequest.h"
#include "Http/Common/HttpResponse.h"
#include "Mongo/Client/MongoProto.h"
#include "Log/Common/Level.h"
#include "Mysql/Common/MysqlProto.h"
#include "Pgsql/Common/PgsqlCommon.h"
#include "Rpc/Lua/LuaServiceTaskSource.h"
#include "Async/Source/WaitTaskSourceBase.h"

#endif
#ifdef __ENABLE_MI_MALLOC__
#include "mimalloc.h"
#include "mimalloc-stats.h"
#endif
#include "Util/Core/Com.h"
namespace acs
{

	NodeSystem::NodeSystem()
		: mNode(nullptr)
	{

	}

    bool NodeSystem::OnInit()
    {
		BIND_RPC_METHOD(NodeSystem::Add);
		BIND_RPC_METHOD(NodeSystem::Del);
		BIND_RPC_METHOD(NodeSystem::Ping);
		BIND_RPC_METHOD(NodeSystem::Hotfix);
		BIND_RPC_METHOD(NodeSystem::RunInfo);
		BIND_RPC_METHOD(NodeSystem::Shutdown);
		BIND_RPC_METHOD(NodeSystem::LoadConfig);
		this->mNode = this->GetComponent<NodeComponent>();
		return true;
    }

	int NodeSystem::Ping(long long id)
    {
        return XCode::Ok;
    }

    int NodeSystem::Add(const json::r::Document & document)
	{
		int id = 0;
		std::string name;
		json::r::Value listenObject;
		LOG_ERROR_CHECK_ARGS(document.Get("id", id))
		LOG_ERROR_CHECK_ARGS(document.Get("name", name))
		LOG_ERROR_CHECK_ARGS(document.Get("listen", listenObject))
		if(this->mApp->Equal(id))
		{
			return XCode::Ok;
		}
		Node * node = this->mNode->Get(id);
		if(node == nullptr)
		{
			this->mNode->Add(std::make_unique<Node>(id, name));
			node = this->mNode->Get(id);
			if(node == nullptr)
			{
				return XCode::Failure;
			}
		}
		for(const char * key : listenObject.GetAllKey())
		{
			std::string address;
			LOG_CHECK_RET_FALSE(listenObject.Get(key, address))
			node->AddListen(key, address);
		}
		help::AddNodeEvent::Trigger(name, id);
		LOG_INFO("[add node] {}", document.ToString())
		return XCode::Ok;
	}

    int NodeSystem::Del(const rpc::Message& request)
	{
		int actorId = 0;
		if(!help::Math::ToNumber(request.GetBody(), actorId))
		{
			return XCode::CallArgsError;
		}
		this->mNode->Remove(actorId);
		help::DelNodeEvent::Trigger(actorId);
		return XCode::Ok;
	}

    int NodeSystem::Shutdown()
	{
		this->mApp->Stop();
		return XCode::Ok;
	}

    int NodeSystem::LoadConfig()
    {
        ConfigComponent * textComponent = this->GetComponent<ConfigComponent>();
        if(textComponent != nullptr)
        {
			textComponent->OnRefresh();
        }
        return XCode::Ok;
    }

	int NodeSystem::RunInfo(json::w::Document & document)
	{
		os::SystemInfo systemInfo;
		os::System::GetSystemInfo(systemInfo);
		long long startMemory = this->mApp->GetStartUseMemory();
		{
			document.Add("name", this->mApp->Name());
			document.Add("id", this->mApp->GetNodeId());
			document.Add("fps", this->mApp->GetFps());
			long long start = this->mApp->StartTime() / 1000;
			document.Add("start_time", help::Time::GetDateString(start));

			std::unique_ptr<json::w::Value> jsonObject = document.AddObject("listen");
			for (const std::pair<std::string, std::string>& item: this->mApp->GetListens())
			{
				jsonObject->Add(item.first.c_str(), item.second);
			}
			document.Add("cpu", fmt::format("{:.3f}%", systemInfo.cpu));
			document.Add("use_memory", help::com::BytesToString(systemInfo.use_memory));
			document.Add("max_memory", help::com::BytesToString(systemInfo.max_memory));
			document.Add("cost_memory", help::com::BytesToString(systemInfo.use_memory - startMemory));
			document.Add("const_memory_b", systemInfo.use_memory - startMemory);
#ifdef __ENABLE_MI_MALLOC__
			std::unique_ptr<json::w::Value> mallocObject = document.AddObject("mimalloc");
			{
				//mi_stats_t stats;
				//mi_stats_get(sizeof(mi_stats_t), &stats);
				// mallocObject->Add("version", stats.version);
				// mallocObject->Add("threads", stats.threads.current);
				// mallocObject->Add("committed", stats.committed.current);
			}
#endif

#ifdef __SHARE_PTR_COUNTER__
			size_t count1 = rpc::Message::GetObjectCount();
			size_t count2 = rpc::InnerTcpClient::GetObjectCount();
			size_t count3 = rpc::OuterTcpSession::GetObjectCount();
			size_t count10 = http::Request::GetObjectCount();
			size_t count11 = http::Response::GetObjectCount();
			size_t count4 = http::Session::GetObjectCount();
			size_t count5 = http::Client::GetObjectCount();
			size_t count6 = acs::App::ActorMgr()->GetActorCount();
			size_t count7 = acs::LuaWaitTaskSource::GetObjectCount();

			size_t count8 = tcp::Socket::GetObjectCount();
			size_t count9 = TaskContext::GetObjectCount();

			size_t count12 = redis::Request::GetObjectCount();
			size_t count13 = redis::Response::GetObjectCount();
			size_t count15 = LuaRpcTaskSource::GetObjectCount();

			size_t count16 = StaticMethod::GetObjectCount();
			size_t count17 = mongo::Request::GetObjectCount();
			size_t count18 = mongo::Response::GetObjectCount();

			size_t count19 = custom::LogInfo::GetObjectCount();

			size_t count20 = mysql::Request::GetObjectCount();
			size_t count21 = mysql::Response::GetObjectCount();

			size_t count22 = pgsql::Request::GetObjectCount();
			size_t count23 = pgsql::Response::GetObjectCount();
			size_t count24 = pgsql::Result::GetObjectCount();

			size_t count25 = LuaServiceTaskSource::GetObjectCount();

			size_t count26 = WaitTaskSourceBase::GetObjectCount();

			std::shared_ptr<json::w::Value> jsonValue = document.AddObject("memory");
			{
				jsonValue->Add("log_info", count19);
				jsonValue->Add("rpc_message", count1);
				jsonValue->Add("inner_client", count2);
				jsonValue->Add("outer_client", count3);
				jsonValue->Add("http_client", count5);
				jsonValue->Add("http_session", count4);
				jsonValue->Add("http_request", count10);
				jsonValue->Add("http_response", count11);
				jsonValue->Add("player_count", count6);
				jsonValue->Add("lua_wait_task", count7);
				jsonValue->Add("lua_rpc_task", count15);
				jsonValue->Add("socket_count", count8);
				jsonValue->Add("coroutine_count", count9);
				jsonValue->Add("redis_request", count12);
				jsonValue->Add("redis_response", count13);
				jsonValue->Add("async_task", count26);

				jsonValue->Add("method", count16);
				jsonValue->Add("mongo_request", count17);
				jsonValue->Add("mongo_response", count18);

				jsonValue->Add("mysql_request", count20);
				jsonValue->Add("mysql_response", count21);

				jsonValue->Add("pgsql_request", count22);
				jsonValue->Add("pgsql_response", count23);
				jsonValue->Add("pgsql_result", count24);

				jsonValue->Add("lua_task_source", count25);

			}
#endif
			std::vector<IServerRecord*> records;
			this->mApp->GetComponents(records);
			for (IServerRecord* record: records)
			{
				timer::ElapsedTimer timer1;
				record->OnRecord(document);
				//LOG_INFO("[{}] {}ms", (dynamic_cast<Component*>(record))->GetName(), timer1.GetMs());
			}
		}
		return XCode::Ok;
	}

    int NodeSystem::Hotfix()
    {
        std::vector<IRefresh *> components;
		this->mApp->GetComponents(components);
        for(IRefresh * component : components)
        {
			component->OnRefresh();
        }
#ifdef __OS_LINUX__
		malloc_trim(0);
#endif
#ifdef __ENABLE_MI_MALLOC__
		mi_collect(true);
#endif
        return XCode::Ok;
    }

	int NodeSystem::Find(const rpc::Message& request)
	{
		return XCode::Ok;
	}
}