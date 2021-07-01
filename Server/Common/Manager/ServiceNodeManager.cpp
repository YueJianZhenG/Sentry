﻿#include "ServiceNodeManager.h"
#include <Other/ServiceNode.h>
#include<Manager/NetWorkManager.h>
namespace SoEasy
{
	bool ServiceNodeManager::DelServiceNode(int nodeId)
	{
		auto iter = this->mServiceNodeMap1.find(nodeId);
		if (iter != this->mServiceNodeMap1.end())
		{
			ServiceNode *serviceNode = iter->second;
			if (serviceNode != nullptr)
			{
				serviceNode->SetActive(false);
				const std::string &address = serviceNode->GetAddress();
				auto iter1 = this->mServiceNodeMap2.find(address);
				if (iter1 != this->mServiceNodeMap2.end())
				{
					this->mServiceNodeMap2.erase(iter1);
				}
			}
			this->mServiceNodeMap1.erase(iter);
			return true;
		}
		return false;
	}

	bool ServiceNodeManager::DelServiceNode(const std::string &address)
	{
		auto iter1 = this->mServiceNodeMap2.find(address);
		if (iter1 != this->mServiceNodeMap2.end())
		{
			ServiceNode *serviceNode = iter1->second;
			if (serviceNode != nullptr)
			{
				serviceNode->SetActive(false);
				const int nodeId = serviceNode->GetNodeId();
				auto iter2 = this->mServiceNodeMap1.find(nodeId);
				if (iter2 != this->mServiceNodeMap1.end())
				{
					this->mServiceNodeMap1.erase(iter2);
				}
			}
			this->mServiceNodeMap2.erase(iter1);
			return true;
		}
		return false;
	}

	bool ServiceNodeManager::AddServiceNode(ServiceNode *serviceNode)
	{
		const int id = serviceNode->GetNodeId();
		const std::string &address = serviceNode->GetAddress();
		auto iter = this->mServiceNodeMap1.find(id);
		if (iter == this->mServiceNodeMap1.end())
		{
			SayNoDebugLog(serviceNode->GetJsonString());
			this->mServiceNodeArray.push_back(serviceNode);
			this->mServiceNodeMap1.emplace(id, serviceNode);
			this->mServiceNodeMap2.emplace(address, serviceNode);
			serviceNode->Init(this->GetApp(), serviceNode->GetNodeName());
			return true;
		}
		return false;
	}

	SharedTcpSession ServiceNodeManager::GetNodeSession(const int nodeId)
	{
		ServiceNode * serviceNode = this->GetServiceNode(nodeId);
		if (serviceNode == nullptr)
		{
			return nullptr;
		}
		const std::string & name = serviceNode->GetNodeName();
		const std::string & address = serviceNode->GetAddress();
		SharedTcpSession nodeSession = this->mNetWorkManager->GetTcpSession(address);
		if (nodeSession == nullptr)
		{
			auto iter = this->mConnectSessionMap.find(address);
			if (iter != this->mConnectSessionMap.end())
			{
				return nullptr;
			}
			nodeSession = this->CreateTcpSession(name, address);
			this->mConnectSessionMap.insert(std::make_pair(address, nodeSession));
			return nullptr;
		}
		return nodeSession->IsActive() ? nodeSession : nullptr;
	}

	bool ServiceNodeManager::OnInit()
	{
		std::string queryAddress;
		SessionManager::OnInit();
		SayNoAssertRetFalse_F(this->GetConfig().GetValue("QueryAddress", queryAddress));
		SayNoAssertRetFalse_F(this->mNetWorkManager = this->GetManager<NetWorkManager>());
		ServiceNode * centerNode = new ServiceNode(0, 0, "Center", queryAddress);
		return centerNode->AddService(std::string("ServiceRegistry")) && this->AddServiceNode(centerNode);
	}

	void ServiceNodeManager::OnSystemUpdate()
	{
		SessionManager::OnSystemUpdate();
		if (!this->mServiceNodeArray.empty())
		{
			auto iter = this->mServiceNodeArray.begin();
			for (; iter != this->mServiceNodeArray.end();)
			{
				ServiceNode *serviceNode = (*iter);
				if (serviceNode == nullptr || !serviceNode->IsActive())
				{
					serviceNode->OnDestory();
					delete serviceNode;
					this->mServiceNodeArray.erase(iter++);
					continue;
				}
				iter++;
				serviceNode->OnSystemUpdate();
			}
		}
	}

	void ServiceNodeManager::OnSessionErrorAfter(SharedTcpSession tcpSession)
	{
	}

	void ServiceNodeManager::OnSessionConnectAfter(SharedTcpSession tcpSession)
	{
		const std::string & address = tcpSession->GetAddress();
		auto iter = this->mConnectSessionMap.find(address);
		if (iter != this->mConnectSessionMap.end())
		{
			this->mConnectSessionMap.erase(iter);
		}
	}

	ServiceNode *ServiceNodeManager::GetServiceNode(const int nodeId)
	{
		auto iter = this->mServiceNodeMap1.find(nodeId);
		if (iter != this->mServiceNodeMap1.end())
		{
			ServiceNode *serviceNode = iter->second;
			if (serviceNode != nullptr && serviceNode->IsActive())
			{
				return serviceNode;
			}
		}
		return nullptr;
	}

	ServiceNode *ServiceNodeManager::GetServiceNode(const std::string &address)
	{
		auto iter = this->mServiceNodeMap2.find(address);
		if (iter != this->mServiceNodeMap2.end())
		{
			ServiceNode *serviceNode = iter->second;
			if (serviceNode != nullptr && serviceNode->IsActive())
			{
				return serviceNode;
			}
		}
		return nullptr;
	}

	ServiceNode *ServiceNodeManager::GetNodeByNodeName(const std::string &nodeName)
	{
		for (ServiceNode *serviceNode : this->mServiceNodeArray)
		{
			if (serviceNode->IsActive() && serviceNode->GetNodeName() == nodeName)
			{
				return serviceNode;
			}
		}
		return nullptr;
	}
	ServiceNode *ServiceNodeManager::GetNodeByServiceName(const std::string &service)
	{
		for (ServiceNode *serviceNode : this->mServiceNodeArray)
		{
			if (serviceNode->IsActive() && serviceNode->HasService(service))
			{
				return serviceNode;
			}
		}
		return nullptr;
	}

}
