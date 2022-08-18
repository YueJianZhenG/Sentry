﻿#include"RedisRegistryComponent.h"
#include"App/App.h"
#include"Network/Listener/TcpServerListener.h"
#include"Network/Listener/TcpServerComponent.h"
#include"Component/Redis/MainRedisComponent.h"
#include"Component/RpcService/LocalService.h"
#include"Component/Redis/RedisSubComponent.h"

namespace Sentry
{
    bool ServiceNode::RemoveService(const std::string &service)
    {
        auto iter = this->mServices.find(service);
        if(iter != this->mServices.end())
        {
            this->mServices.erase(iter);
            return true;
        }
        return false;
    }
}

namespace Sentry
{
	bool RedisRegistryComponent::LateAwake()
	{
        this->mTaskComponent = this->GetComponent<TaskComponent>();
        this->mSubComponent = this->GetComponent<RedisSubComponent>();
        this->mRedisComponent = this->GetComponent<MainRedisComponent>();
		LOG_CHECK_RET_FALSE(this->GetConfig().GetMember("area_id", this->mAreaId));
		LOG_CHECK_RET_FALSE(this->GetConfig().GetMember("node_name", this->mNodeName));
		LOG_CHECK_RET_FALSE(this->GetConfig().GetListener("rpc", this->mRpcAddress));
		return true;
	}

	bool RedisRegistryComponent::OnRegisterEvent(NetEventRegistry& eventRegister)
	{
        eventRegister.Sub("Ping", &RedisRegistryComponent::Ping, this);
        eventRegister.Sub("AddNode", &RedisRegistryComponent::AddNode, this);
		eventRegister.Sub("AddService", &RedisRegistryComponent::AddService, this);
        eventRegister.Sub("RemoveService", &RedisRegistryComponent::RemoveService, this);
        return true;
	}

    bool RedisRegistryComponent::Ping(const Json::Reader &json)
    {
        std::string address;
        LOG_CHECK_RET_FALSE(json.GetMember("address", address));
        auto iter = this->mNodes.find(address);
        if(iter == this->mNodes.end())
        {
            return false;
        }
        iter->second->UpdateTime();
        LOG_INFO("[" << address << "] update time");
        return true;
    }

    void RedisRegistryComponent::OnSecondUpdate(const int tick)
    {
        if(tick % 10 == 0)
        {
           SharedRedisClient redisClient = this->mRedisComponent->GetClient("main");
           if(redisClient != nullptr)
           {
               Json::Writer json;
               json << "address" << this->mRpcAddress;
               std::shared_ptr<RedisRequest> request = RedisRequest::Make(
                       "PUBLISH", "RedisRegistryComponent.Ping", json.JsonString());
               redisClient->SendCommand(request);
           }
           auto iter = this->mNodes.begin();
           long long nowTime = Helper::Time::GetNowSecTime();
           for(; iter != this->mNodes.end(); iter++)
           {
               ServiceNode * node = iter->second;
               if(nowTime - node->GetLastTime() >= 20)
               {
                   LOG_ERROR(node->GetHost() << " time out");
               }
           }
        }
    }

	void RedisRegistryComponent::OnAddService(Component* component)
	{
		if(component->Cast<Service>())
		{
            const std::string & name = component->GetName();
            SharedRedisClient redisClient = this->mRedisComponent->GetClient("main");
            if(redisClient != nullptr)
            {
                std::shared_ptr<RedisRequest> cmd =
                        RedisRequest::Make("SADD", this->mRpcAddress, name);
                redisClient->SendCommand(cmd);
            }
			Json::Writer jsonWriter;
			jsonWriter << "address" << this->mRpcAddress << "service" << component->GetName();
            const std::string message = jsonWriter.JsonString();
            std::shared_ptr<RedisRequest> request =
                    RedisRequest::Make("PUBLISH", "RedisRegistryComponent.AddService", message);
            redisClient->SendCommand(request);
		}
	}

	void RedisRegistryComponent::OnDelService(Component* component)
	{
        if(component->Cast<Service>())
        {
            const std::string &name = component->GetName();
            SharedRedisClient redisClient = this->mRedisComponent->GetClient("main");
            if (redisClient != nullptr)
            {
                std::shared_ptr<RedisRequest> cmd =
                        RedisRequest::Make("SREM", this->mRpcAddress, name);
                redisClient->SendCommand(cmd);
            }
            Json::Writer jsonWriter;
            jsonWriter << "address" << this->mRpcAddress << "service" << component->GetName();
            const std::string message = jsonWriter.JsonString();
            std::shared_ptr<RedisRequest> request =
                    RedisRequest::Make("PUBLISH", "RedisRegistryComponent.RemoveService", message);
            redisClient->SendCommand(request);
        }
	}

    bool RedisRegistryComponent::RemoveService(const Json::Reader &json)
    {
        std::string address, service;
        LOG_CHECK_RET_FALSE(json.GetMember("address", address));
        LOG_CHECK_RET_FALSE(json.GetMember("service", service));
        Service * component = this->GetApp()->GetService(service);
        if(component != nullptr)
        {
            component->DelHost(address);
        }
        auto iter = this->mNodes.find(address);
        if(iter != this->mNodes.end())
        {
            iter->second->RemoveService(service);
        }
        return true;
    }

	void RedisRegistryComponent::OnComplete()//通知其他服务器 我加入了
    {
        std::shared_ptr<RedisResponse> response1 =
                this->mRedisComponent->RunCmd("main", "SMEMBERS", this->mRpcAddress);
        if(response1 != nullptr && response1->GetArraySize() > 0)
        {
            std::shared_ptr<RedisRequest> redisRequest(new RedisRequest("SREM"));
            for(size_t index = 0; index < response1->GetArraySize(); index++)
            {
                const RedisString * redisString = response1->Get(index)->Cast<RedisString>();
                if(redisString != nullptr)
                {
                    redisRequest->AddParameter(redisString->GetValue());
                }
            }
            this->mRedisComponent->Run("name", redisRequest);
        }
        Json::Writer json;
        std::shared_ptr<RedisRequest> redisRequest(new RedisRequest("SADD"));

        json.BeginArray("services");
        std::vector<Service *> components;
        redisRequest->AddParameter(this->mRpcAddress);
        this->GetApp()->GetServices(components);
        for (Service *component: components)
        {
            if (component->IsStartService())
            {
                json << component->GetName();
                redisRequest->AddParameter(component->GetName());
            }
        }

        this->mRedisComponent->Run("main", redisRequest);

        json << Json::End::EndArray;
        json << "address" << this->mRpcAddress;
        const std::string message = json.JsonString();
        const std::string channel = "RedisRegistryComponent.AddNode";
        std::shared_ptr<RedisResponse> response2 =
                this->mRedisComponent->RunCmd("main", "PUBLISH", channel, message);
        if(response2 != nullptr && response2->GetNumber() > 0)
        {
            LOG_INFO("publish channel number = " << response2->GetNumber());
        }
        this->QueryAllNodes();
    }

    void RedisRegistryComponent::QueryAllNodes()
    {
        std::shared_ptr<RedisResponse> response =
                this->mRedisComponent->RunCmd("main", "PUBSUB", "CHANNELS", "*:*");
        if(response != nullptr && response->GetArraySize() > 0)
        {
            for(size_t index = 0; index < response->GetArraySize(); index++)
            {
                const RedisString * redisString = response->Get(index)->Cast<RedisString>();
                std::shared_ptr<RedisResponse> response1 =
                        this->mRedisComponent->RunCmd("main", "SMEMBERS", redisString->GetValue());
                for(size_t x = 0; x < response1->GetArraySize(); x ++)
                {
                    redisString = response1->Get(x)->Cast<RedisString>();
                    Service * service = this->GetApp()->GetService(redisString->GetValue());
                    if(service != nullptr)
                    {
                        service->AddHost(redisString->GetValue());
                    }
                }
            }
        }
    }

	bool RedisRegistryComponent::AddNode(const Json::Reader &json)
	{
        std::string address;
		std::vector<std::string> services;
        LOG_CHECK_RET_FALSE(json.GetMember("address", address));
        LOG_CHECK_RET_FALSE(json.GetMember("services", services));
        auto iter = this->mNodes.find(address);
        if(iter == this->mNodes.end())
        {
            ServiceNode * node = new ServiceNode(address);
            this->mNodes.emplace(address, node);
        }
        ServiceNode * node = this->mNodes[address];
		for(const std::string & service : services)
		{
			Service * localRpcService = this->GetApp()->GetService(service);
			if(localRpcService != nullptr)
			{
				localRpcService->AddHost(address);
			}
            node->AddService(service);
		}
		return true;
	}

	bool RedisRegistryComponent::AddService(const Json::Reader &json)
	{
		std::string address, service;
		LOG_CHECK_RET_FALSE(json.GetMember("address", address));
		LOG_CHECK_RET_FALSE(json.GetMember("service", service));
        auto iter = this->mNodes.find(address);
        assert(iter != this->mNodes.end());
        ServiceNode * node = iter->second;
		Service * localRpcService = this->GetApp()->GetService(service);
		if(localRpcService != nullptr)
		{
            node->AddService(service);
			localRpcService->AddHost(address);
			return true;
		}
		return false;
	}

	void RedisRegistryComponent::OnDestory()
	{
		std::vector<Service*> compontns;
		this->GetApp()->GetServices(compontns);
		for(Service * component : compontns)
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