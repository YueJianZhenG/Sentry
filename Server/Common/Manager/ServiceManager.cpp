#include"ServiceManager.h"
#include<Service/LocalService.h>
#include<Service/ProxyService.h>

#include<Util/StringHelper.h>
#include<Core/ObjectRegistry.h>

#include<Manager/ListenerManager.h>
#include<Coroutine/CoroutineManager.h>

#include<NetWork/ActionScheduler.h>
namespace SoEasy
{
	
	bool ServiceManager::OnInit()
	{
		SayNoAssertRetFalse_F(SessionManager::OnInit());
		SayNoAssertRetFalse_F(this->GetConfig().GetValue("Service", this->mServiceList));
		
		SayNoAssertRetFalse_F(this->mCorManager = this->GetManager<CoroutineManager>());
		return this->CreateLocalService();
	}

	void ServiceManager::OnInitComplete()
	{
		auto iter = this->mLocalServiceMap.begin();
		for (; iter != this->mLocalServiceMap.end(); iter++)
		{
			LocalService * localService = iter->second;
			auto cb = BIND_ACTION_0(LocalService::OnInitComplete, localService);
			this->mCorManager->Start(localService->GetServiceName() + ".Init", cb);
		}
	}

	void ServiceManager::OnSystemUpdate()
	{
		SessionManager::OnSystemUpdate();
		auto iter = this->mLocalServiceMap.begin();
		for (; iter != this->mLocalServiceMap.end(); iter++)
		{
			LocalService * localService = iter->second;
			if (localService->IsActive())
			{
				localService->OnSystemUpdate();
			}
		}
		auto iter1 = this->mProxyServiceMap.begin();
		for (; iter1 != this->mProxyServiceMap.end(); iter1++)
		{
			ProxyService * proxyService = iter1->second;
			if (proxyService->IsActive())
			{
				proxyService->OnSystemUpdate();
			}
		}
	}

	void ServiceManager::OnSessionErrorAfter(SharedTcpSession tcpSession)
	{

	}

	void ServiceManager::OnSessionConnectAfter(SharedTcpSession tcpSession)
	{
		auto iter1 = this->mLocalServiceMap.begin();
		for (; iter1 != this->mLocalServiceMap.end(); iter1++)
		{
			LocalService * localService = iter1->second;
			localService->OnConnectDone(tcpSession);
		}
		auto iter2 = this->mProxyServiceMap.begin();
		for (; iter2 != this->mProxyServiceMap.end(); iter2++)
		{
			ProxyService * proxyService = iter2->second;
			proxyService->OnConnectDone(tcpSession);
		}
	}

	bool ServiceManager::NewLocalService(const std::string & name)
	{
		LocalService * service = ObjectRegistry<LocalService>::Create(name);
		if (service == nullptr)
		{
			SayNoDebugError("create " << name << " service fail");
			return false;
		}
		
		if (!service->OnInit())
		{
			SayNoDebugError("init " << name << " service fail");
			return false;
		}
		SayNoDebugLog("add new service " << name);
		this->mLocalServiceMap.emplace(name, service);	
		return true;
	}

	LocalService * ServiceManager::GetLocalService(const std::string & name)
	{
		auto iter = this->mLocalServiceMap.find(name);
		return iter != this->mLocalServiceMap.end() ? iter->second : nullptr;
	}

	SharedTcpSession ServiceManager::GetProxySession(const std::string & address)
	{
		auto iter = this->mActionSessionMap.find(address);
		if (iter == this->mActionSessionMap.end())
		{
			std::string ip;
			unsigned short port;
			StringHelper::ParseIpAddress(address, ip, port);
			SharedTcpSession tcpSession = make_shared<TcpClientSession>(this, "ServiceSession", ip, port);
			if (tcpSession->StartConnect())
			{
				this->mActionSessionMap.emplace(address, tcpSession);
				return tcpSession;
			}
			return nullptr;
		}
		return iter->second;
	}

	bool ServiceManager::RemoveProxyervice(const int id)
	{
		auto iter = this->mProxyServiceMap.find(id);
		if (iter != this->mProxyServiceMap.end())
		{
			iter->second->SetActive(false);
			return true;
		}
		return false;
	}

	void ServiceManager::GetLocalServices(std::vector<LocalService*> & services)
	{
		services.clear();
		auto iter = this->mLocalServiceMap.begin();
		for (; iter != this->mLocalServiceMap.end(); iter++)
		{
			services.push_back(iter->second);
		}
	}

	void ServiceManager::AddProxyService(const std::string & name, int areaId, int serviceId, const std::string & address)
	{
		auto iter = this->mProxyServiceMap.find(serviceId);
		if (iter == this->mProxyServiceMap.end())
		{
			ProxyService * proxyService = new ProxyService(name, address, serviceId, areaId);
			if (proxyService != nullptr)
			{
				proxyService->Init(this->GetApp(), name);
				this->mProxyServiceMap.emplace(serviceId, proxyService);
				this->mCorManager->Start(name + ".Init", BIND_ACTION_0(ProxyService::OnInitComplete, proxyService));
			}
		}
	}

	ProxyService * ServiceManager::GetProxyService(int id)
	{
		auto iter = this->mProxyServiceMap.find(id);
		return iter != this->mProxyServiceMap.end() ? iter->second : nullptr;
	}

	ProxyService * ServiceManager::GetProxyService(const std::string & name)
	{
		return nullptr;
	}

	bool ServiceManager::CreateLocalService()
	{
		Applocation * app = Applocation::Get();
		for (size_t index = 0; index < this->mServiceList.size(); index++)
		{
			const std::string & name = this->mServiceList[index];
			if (this->NewLocalService(name) == false)
			{
				return false;
			}
		}
		return true;
	}

}
