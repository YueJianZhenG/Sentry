//
// Created by MyPC on 2023/4/15.
//
#include"XCode/XCode.h"
#include"Auth/Jwt/Jwt.h"
#include"MasterComponent.h"
#include"Entity/Actor/App.h"
#include"Server/Config/CodeConfig.h"

#include"Message/com/com.pb.h"
#include"Message/s2s/registry.pb.h"
#include"Cluster/Config/ClusterConfig.h"
#include"Http/Common/HttpRequest.h"
#include"Http/Common/HttpResponse.h"
#include"Http/Component/HttpComponent.h"


namespace joke
{
	MasterComponent::MasterComponent()
	{
		this->mHttp = nullptr;
		this->mActComponent = nullptr;
	}

	bool MasterComponent::LateAwake()
	{
		std::unique_ptr<json::r::Value> jsonObject;
		if(this->mApp->Config().Get("master", jsonObject))
		{
			jsonObject->Get("host", this->mHost);
		}
		std::string key = this->mApp->Config().GetSecretKey();
		{
			json::w::Document document;
			{
				document.Add("t", 0);
				document.Add("p", http::PermissAdmin);
				document.Add("o", this->mApp->GetSrvId());
			}
			std::string data;
			document.Encode(&data);
#ifdef __ENABLE_OPEN_SSL__
			this->mToken = jwt::Create(data, key);
#else
			LOG_CHECK_RET_FALSE(jsonObject->Get("token", this->mToken))
#endif
		}
		this->mActComponent = this->mApp->ActorMgr();
		this->mHttp = this->GetComponent<HttpComponent>();
		return true;
	}

	bool MasterComponent::RegisterServer() const
	{
		std::string message;
		this->mApp->EncodeToJson(&message);
		std::string url = fmt::format("{}/master/push", this->mHost);
		std::unique_ptr<http::Request> request(new http::Request("POST"));
		{
			request->SetUrl(url);
			request->SetTimeout(5);
			request->SetContent(http::Header::JSON, message);
			request->Header().Add(http::Header::Auth, this->mToken);
		}
		http::Response * response = this->mHttp->Do(std::move(request));
		if(response == nullptr || response->GetBody() == nullptr)
		{
			return false;
		}
		int code = 0;
		const http::Data * httpData = response->GetBody();
		const http::JsonData * jsonData = dynamic_cast<const http::JsonData*>(httpData);
		return jsonData != nullptr && jsonData->Get("code", code) && code == XCode::Ok;
	}

	void MasterComponent::Complete()
	{
		int count = 0;
		CoroutineComponent* coroutine = this->mApp->Coroutine();
		while (!this->RegisterServer())
		{
			count++;
			if (count <= 3)
			{
				coroutine->Sleep(2000);
				continue;
			}
			coroutine->Start(&App::Stop, this->mApp);
			LOG_ERROR("register to center fail count={}", count);
			return;
		}

		std::vector<std::string> servers;
		ClusterConfig::Inst()->GetServers(servers);
		for (const std::string& name: servers)
		{
			while (!this->mActComponent->Hash(name, 0))
			{
				this->SyncServer();
				this->mApp->Coroutine()->Sleep(1000);
				LOG_WARN("===== wait {} start =====", name);
			}
		}
	}

	bool MasterComponent::SyncServer(long long id)
	{
		int appId = this->mApp->GetSrvId();
		std::string url = fmt::format("{}/master/find?id={}&app={}", this->mHost, id, appId);
		std::unique_ptr<http::Request> request(new http::Request("GET"));
		{
			request->SetUrl(url);
			request->SetTimeout(5);
			request->Header().Add(http::Header::Auth, this->mToken);
		}
		http::Response * response = this->mHttp->Do(std::move(request));
		if(response == nullptr || response->GetBody() == nullptr)
		{
			return false;
		}
		int code = 0;
		const http::Data * httpData = response->GetBody();
		const http::JsonData * jsonData = dynamic_cast<const http::JsonData*>(httpData);
		return jsonData != nullptr && jsonData->Get("code", code) && code == XCode::Ok;
	}
}