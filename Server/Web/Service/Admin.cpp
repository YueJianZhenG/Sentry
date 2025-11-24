//
// Created by zmhy0073 on 2022/8/29.
//
#include "Admin.h"
#include "Util/Tools/Zip.h"
#include "Core/System/System.h"
#include "Entity/Actor/App.h"
#include "Message/com/com.pb.h"
#include "Util/File/FileHelper.h"
#include "Util/Tools/TimeHelper.h"
#include "Server/Config/CodeConfig.h"
#include "Timer/Timer/ElapsedTimer.h"
#include "Proto/Component/ProtoComponent.h"
namespace acs
{
	Admin::Admin()
	{
		this->mProto = nullptr;
		this->mActor = nullptr;
	}

    bool Admin::OnInit()
	{
		BIND_COMMON_HTTP_METHOD(Admin::Info);
		BIND_COMMON_HTTP_METHOD(Admin::Ping);
		BIND_COMMON_HTTP_METHOD(Admin::Stop);
		BIND_COMMON_HTTP_METHOD(Admin::Hello);
		BIND_COMMON_HTTP_METHOD(Admin::Hotfix);
		BIND_COMMON_HTTP_METHOD(Admin::AllInfo);
		BIND_COMMON_HTTP_METHOD(Admin::RpcInterface);
		BIND_COMMON_HTTP_METHOD(Admin::HttpInterface);

		this->mProto = this->GetComponent<ProtoComponent>();
		this->mActor = this->GetComponent<NodeComponent>();
		return true;
	}

	int Admin::Ping(const http::FromContent & request, json::w::Value & response)
	{
		int id = 0;
		Actor* node = App::Inst();
		if(request.Get("id", id))
		{
			node = this->mActor->Get(id);
			if(node == nullptr)
			{
				return XCode::NotFoundActor;
			}
		}
		long long t1 = help::Time::NowMil();
		int code = node->Call("NodeSystem.Ping");
		{
			response.Add("time", help::Time::NowMil() - t1);
		}
		return code;
	}

    int Admin::Hello(http::Response& response)
	{
		response.SetContent(http::Header::TEXT, "hello");
		return XCode::Ok;
	}

	void Admin::AddRpcData(json::w::Value & response, const acs::RpcMethodConfig* methodConfig)
	{
		pb_json::JsonOptions options;
		options.add_whitespace = true;
		options.always_print_primitive_fields = true;
		response.Add("name", methodConfig->fullname.c_str());
		response.Add("service", methodConfig->service.c_str());

		switch(methodConfig->proto)
		{
			case rpc::proto::pb:
			{
				std::string json;
				pb::Message * request = this->mProto->Temp(methodConfig->request);
				if(request && pb_json::MessageToJsonString(*request, &json, options).ok())
				{
					response.Add("req", json.c_str(), json.size());
				}
				response.Add("request", methodConfig->request.c_str());
				break;
			}
		}
		if(!methodConfig->request.empty())
		{
			std::string json;
			pb::Message * request = this->mProto->Temp(methodConfig->request);
			if(request && pb_json::MessageToJsonString(*request, &json, options).ok())
			{
				response.Add("req", json.c_str(), json.size());
			}
			response.Add("request", methodConfig->request.c_str());
		}

		if(!methodConfig->response.empty())
		{
			std::string json;
			pb::Message * request = this->mProto->Temp(methodConfig->response);
			if(request && pb_json::MessageToJsonString(*request, &json, options).ok())
			{
				response.Add("res", json.c_str(), json.size());
			}
			response.Add("response", methodConfig->response);
		}

		response.Add("open", methodConfig->open);
		response.Add("async", methodConfig->async);
		response.Add("client", methodConfig->client);
		response.Add("forward", methodConfig->forward);
	}

	int Admin::RpcInterface(const http::FromContent & request, json::w::Document & response)
	{
		int page = 0;
		int type = 0;
		std::string service;
		request.Get("type", type);
		std::vector<const RpcMethodConfig *> methodConfigs;
		if(request.Get("name", service) && !service.empty())
		{
			RpcConfig::Inst()->GetMethodConfigs(service, methodConfigs);
		}
		else
		{
			if(!request.Get("page", page) || page < 0)
			{
				return XCode::CallArgsError;
			}
			page = page - 1;
			RpcConfig::Inst()->GetMethodConfigs(methodConfigs, type);
		}

		std::unique_ptr<json::w::Value> list = response.AddArray("list");
		for(int index = page * 10; index < (page * 10) + 10 && index < methodConfigs.size(); index++)
		{
			const RpcMethodConfig * methodConfig = methodConfigs[index];
			if(methodConfig != nullptr)
			{
				this->AddRpcData(*list->AddObject(), methodConfig);
			}
		}

		response.Add("count", (int)methodConfigs.size());
		return XCode::Ok;
	}

	int Admin::HttpInterface(const http::FromContent & request, json::w::Document & response)
	{
		int page = 0;
		std::string method, name;
		std::vector<const HttpMethodConfig *> configs;
		if(request.Get("name", name) && !name.empty())
		{
			HttpConfig::Inst()->GetMethodList(configs, name);
		}
		else
		{
			request.Get("type", method);
			if(!request.Get("page", page) || page < 0)
			{
				return XCode::CallArgsError;
			}
			page--;
			HttpConfig::Inst()->GetMethodConfigs(configs, method);
		}

		std::unique_ptr<json::w::Value> list = response.AddArray("list");
		for(int index = page * 10; index < page * 10 + 10 && index < configs.size(); index++)
		{
			const HttpMethodConfig * methodConfig = configs.at(index);
			if(methodConfig != nullptr)
			{
				std::unique_ptr<json::w::Value> obj = list->AddObject();
				{
					obj->Add("url", methodConfig->path);
					obj->Add("desc", methodConfig->desc);
					obj->Add("method", methodConfig->type);
					obj->Add("async", methodConfig->async);
					obj->Add("access", methodConfig->access);
					obj->Add("content", methodConfig->content);
					obj->Add("request", methodConfig->request);
					obj->Add("bind", fmt::format("{}.{}", methodConfig->service, methodConfig->method));
				}
			}
		}
		response.Add("count", (int)configs.size());
		return XCode::Ok;
	}

	int Admin::Hotfix(const http::FromContent & request, json::w::Value&response)
	{
		int id = 0;
		if(!request.Get("id", id))
		{
			return XCode::CallArgsError;
		}
		Actor* server = this->mActor->Get(id);
		if(server == nullptr)
		{
			return XCode::NotFoundActor;
		}
		return server->Call("NodeSystem.Hotfix");
	}

	int Admin::Stop(const http::FromContent & request, json::w::Value & response)
	{
		int id = 0;
		if(!request.Get("id", id))
		{
			response.Add("error", "not field:id");
			return XCode::CallArgsError;
		}
		Actor* server = this->mActor->Get(id);
		if(server == nullptr)
		{
			response.Add("error", "not find server");
			return XCode::NotFoundActor;
		}
		return server->Call("NodeSystem.Shutdown");
	}

	int Admin::Info(const http::FromContent& request, json::w::Document& response)
	{
		const std::string func("NodeSystem.RunInfo");
		std::unique_ptr<json::r::Document> result = std::make_unique<json::r::Document>();
		{
			int code = this->mApp->Call(func, result);
			if(code != XCode::Ok)
			{
				return code;
			}
			response.Add("data", *result);
		}
		return XCode::Ok;
	}

	int Admin::AllInfo(const http::FromContent & request, json::w::Document& response)
    {
		int id = -1;
		std::vector<int> serverActors;
		if(request.Get("id", id) && id >= 0)
		{
			serverActors.emplace_back(id);
		}
		else
		{
			serverActors = this->mActor->GetNodes();
			serverActors.emplace_back(this->mApp->GetNodeId());
		}
		std::string func("NodeSystem.RunInfo");
		std::unique_ptr<json::w::Value> document = response.AddArray("list");
		std::unique_ptr<json::r::Document> result = std::make_unique<json::r::Document>();
		for(const int serverId : serverActors)
		{
			if(Actor * server = this->mActor->Get(serverId))
			{
				timer::ElapsedTimer timer1;
				int code = server->Call(func, result);
				if (code == XCode::Ok)
				{
					std::unique_ptr<json::w::Value> jsonValue;
					if(document->PushObject(*result, jsonValue))
					{
						jsonValue->Add("timeout", fmt::format("{}ms", timer1.GetMs()));
					}
				}
				else
				{
					const std::string & desc = CodeConfig::Inst()->GetDesc(code);
					std::unique_ptr<json::w::Value> jsonObject = document->AddObject();
					{
						jsonObject->Add("id", serverId);
						jsonObject->Add("error", desc);
						jsonObject->Add("name", server->Name());
					}
					LOG_ERROR("call [{}.{}] code:{} => {}" , serverId, func, code, desc)
				}
			}
		}
		return XCode::Ok;
    }
}