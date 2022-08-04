﻿#include"ServiceMgrComponent.h"
#include"App/App.h"
#include"Network/Listener/TcpServerListener.h"
#include"Network/Listener/TcpServerComponent.h"
#include"Component/Redis/MainRedisComponent.h"
#include"Component/RpcService/LocalServiceComponent.h"

namespace Sentry
{
	bool ServiceMgrComponent::LateAwake()
	{
		this->mRedisComponent = this->GetComponent<MainRedisComponent>();
		LOG_CHECK_RET_FALSE(this->GetConfig().GetMember("area_id", this->mAreaId));
		LOG_CHECK_RET_FALSE(this->GetConfig().GetMember("node_name", this->mNodeName));
		LOG_CHECK_RET_FALSE(this->GetConfig().GetListener("rpc", this->mRpcAddress));
		return true;
	}

	bool ServiceMgrComponent::OnRegisterEvent(NetEventRegistry& eventRegister)
	{
		eventRegister.Sub("OnServiceAdd", &ServiceMgrComponent::OnServiceAdd, this);
		eventRegister.Sub("OnServiceDel", &ServiceMgrComponent::OnServiceDel, this);
		eventRegister.Sub("OnNodeRegister", &ServiceMgrComponent::OnNodeRegister, this);
		return true;
	}

	void ServiceMgrComponent::OnAddService(Component* component)
	{
		if(component->Cast<ServiceComponent>())
		{
			Json::Writer jsonWriter;
            std::shared_ptr<Json::Reader> response(new Json::Reader());
			jsonWriter << "address" << this->mRpcAddress << "service" << component->GetName();
			if(!this->mRedisComponent->Call("main", "node.add", jsonWriter, response))
			{
				LOG_ERROR("call node.add failure " << component->GetName());
			}
		}
	}

	void ServiceMgrComponent::OnDelService(Component* component)
	{

	}

	bool ServiceMgrComponent::QueryNodeInfo(const std::string& address)
	{
		Json::Writer jsonWriter;
		jsonWriter << "address" << address;
		std::shared_ptr<Json::Reader> jsonResponse(new Json::Reader());
		if(!this->mRedisComponent->Call("main", "node.query", jsonWriter, jsonResponse))
		{
			LOG_ERROR("query " << address << " node info error");
			return false;
		}
		std::string jsonMessage;
		if(!jsonResponse->GetMember("json", jsonMessage))
		{
			LOG_ERROR("query " << address << " json message failure");
			return false;
		}
		Json::Reader jsonReader;
		if(!jsonReader.ParseJson(jsonMessage))
		{
			LOG_ERROR("parse node json info error");
			return false;
		}
		std::vector<std::string> jsonServices;
		if(jsonReader.GetMember("services", jsonServices))
		{
			for(const std::string & service : jsonServices)
			{
				ServiceComponent * component = this->GetApp()->GetService(service);
				if(component != nullptr && !component->GetAddressProxy().HasAddress(address))
				{
					component->GetAddressProxy().AddAddress(address);
				}
			}
		}
		return true;
	}

	void ServiceMgrComponent::OnComplete()//通知其他服务器 我加入了
	{
		Json::Writer json;
		json.BeginArray("services");
		std::vector<ServiceComponent*> components;
		this->GetApp()->GetServices(components);
		for (ServiceComponent* component: components)
		{
			if(component->IsStartService())
			{
				json << component->GetName();
			}
		}
		json << Json::End::EndArray;
        json << "broacast" << true;
        json << "address" << this->mRpcAddress;
		std::shared_ptr<Json::Reader> response(new Json::Reader());
		if(!this->mRedisComponent->Call("main", "node.update", json, response))
		{
			LOG_ERROR("register failure");
			return;
		}
		this->mJsonMessages.clear();
		response->GetMember("nodes", this->mJsonMessages);
		for(const std::string & address : this->mJsonMessages)
		{
			if(!this->QueryNodeInfo(address))
			{
				this->RemoveAddress(address);
			}
		}
	}

	void ServiceMgrComponent::RemoveAddress(const std::string & address)
	{
		std::vector<ServiceComponent *> services;
		this->GetApp()->GetServices(services);
		for(ServiceComponent * component : services)
		{
			if(component->GetAddressProxy().DelAddress(address))
			{
				LOG_ERROR("remove [" << address << "] from " << component->GetName());
			}
		}
	}


	bool ServiceMgrComponent::OnNodeRegister(const Json::Reader& json)
	{
		std::string address;
		std::vector<std::string> services;
		LOG_CHECK_RET_FALSE(json.GetMember("address", address));
		LOG_CHECK_RET_FALSE(json.GetMember("services", services));
		for(const std::string & service : services)
		{
			ServiceComponent * localRpcService = this->GetApp()->GetService(service);
			if(localRpcService != nullptr)
			{
				localRpcService->GetAddressProxy().AddAddress(address);
			}
		}
		this->mAllAddress.insert(address);
		return true;
	}

	bool ServiceMgrComponent::OnServiceAdd(const Json::Reader& json)
	{
		std::string address, service;
		LOG_CHECK_RET_FALSE(json.GetMember("address", address));
		LOG_CHECK_RET_FALSE(json.GetMember("service", service));
		ServiceComponent * localRpcService = this->GetApp()->GetService(service);
		if(localRpcService != nullptr)
		{
			localRpcService->GetAddressProxy().AddAddress(address);
			return true;
		}
		return false;
	}

	bool ServiceMgrComponent::OnServiceDel(const Json::Reader& json)
	{
		std::string address, service;
		LOG_CHECK_RET_FALSE(json.GetMember("address", address));
		LOG_CHECK_RET_FALSE(json.GetMember("service", service));
		ServiceComponent* localRpcService = this->GetApp()->GetService(service);
		return localRpcService != nullptr && localRpcService->GetAddressProxy().DelAddress(address);
	}

	bool ServiceMgrComponent::RefreshService()
	{
		Json::Writer json;
		json.BeginArray("services");
		std::vector<ServiceComponent*> components;
		this->GetApp()->GetServices(components);
		for (ServiceComponent* component: components)
		{
			json << component->GetName();
		}
		json << Json::End::EndArray;
		json << "address" << this->mRpcAddress;
		std::shared_ptr<Json::Reader> response(new Json::Reader());
		if (!this->mRedisComponent->Call("main", "node.refresh", json, response))
		{
			return false;
		}
		this->mJsonMessages.clear();
		response->GetMember("services", this->mJsonMessages);
		for (const std::string& jsonMessage: this->mJsonMessages)
		{
			Json::Reader jsonReader;
			if (!jsonReader.ParseJson(jsonMessage))
			{
				LOG_ERROR("parse json message failure");
				return false;
			}
			std::string address;
			this->mServices.clear();
			jsonReader.GetMember("address", address);
			jsonReader.GetMember("services", this->mServices);
			for (const std::string& service: this->mServices)
			{
				ServiceComponent* localRpcService = this->GetApp()->GetService(service);
				if (localRpcService != nullptr)
				{
					localRpcService->GetAddressProxy().Clear();
					localRpcService->GetAddressProxy().AddAddress(address);
				}
			}
		}
		return true;
	}

	void ServiceMgrComponent::OnDestory()
	{
		std::vector<ServiceComponent*> compontns;
		this->GetApp()->GetServices(compontns);
		for(ServiceComponent * component : compontns)
		{
			if(component->IsStartService())
			{
				Json::Writer jsonWriter;
                std::shared_ptr<Json::Reader> response(new Json::Reader());
                jsonWriter << "address" << this->mRpcAddress << "service" << component->GetName();
				this->mRedisComponent->Call("main", "node.del", jsonWriter, response);
			}
		}
	}
}// namespace Sentry