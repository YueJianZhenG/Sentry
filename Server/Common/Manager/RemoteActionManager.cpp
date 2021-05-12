#include"RemoteActionManager.h"
#include<Util/StringHelper.h>
#include<NetWork/ActionScheduler.h>
#include<NetWork/RemoteScheduler.h>
#include<Coroutine/CoroutineManager.h>
#include<Manager/LocalActionManager.h>
#include<Manager/ListenerManager.h>
namespace SoEasy
{
	RemoteActionManager::RemoteActionManager()
	{
		this->mActionManager = nullptr;
		this->mListenerManager = nullptr;
		this->mCoroutineManager = nullptr;
	}

	bool RemoteActionManager::OnInit()
	{
		if (!SessionManager::OnInit())
		{
			return false;
		}
		if (!this->GetConfig().GetValue("AreaId", this->mAreaId))
		{
			SayNoDebugError("not find field AreaId");
			return false;
		}
		std::string connectAddress;
		if (!this->GetConfig().GetValue("ActionQueryAddress", connectAddress))
		{
			SayNoDebugError("not find field ActionQueryAddress");
			return false;
		}
		if (!StringHelper::ParseIpAddress(connectAddress, mQueryIp, mQueryPort))
		{
			SayNoDebugFatal("parse ActionQueryAddress fail");
			return false;
		}
		this->mActionManager = this->GetManager<LocalActionManager>();
		this->mListenerManager = this->GetManager<ListenerManager>();
		this->mCoroutineManager = this->GetManager<CoroutineManager>();
		SayNoAssertRetFalse_F(this->mActionManager);
		SayNoAssertRetFalse_F(this->mListenerManager);
		SayNoAssertRetFalse_F(this->mCoroutineManager);

		this->mActionQuerySession = make_shared<TcpClientSession>(this, "QuerySession", mQueryIp, mQueryPort);
		return this->mActionQuerySession->StartConnect();
	}

	void RemoteActionManager::OnSessionErrorAfter(shared_ptr<TcpClientSession> tcpSession)
	{
		
	}

	void RemoteActionManager::OnSessionConnectAfter(shared_ptr<TcpClientSession> tcpSession)
	{
		if (tcpSession == this->mActionQuerySession)
		{
			this->StartRegisterAction();
			SayNoDebugInfo("start register location action to " << tcpSession->GetAddress());
		}
		else
		{
			const std::string & address = tcpSession->GetAddress();
			std::vector<shared_ptr<RemoteActionProxy>> actionProxys;
			this->GetActionProxyByAddress(address, actionProxys);
			for (size_t index = 0; index < actionProxys.size(); index++)
			{
				shared_ptr<RemoteActionProxy> actionProxy = actionProxys[index];
				actionProxy->BindSession(tcpSession);
			}
		}
	}

	bool RemoteActionManager::GetActionProxy(const std::string & action, shared_ptr<RemoteActionProxy> & actionProxy)
	{
		auto iter = this->mActionProxyMap.find(action);
		if (iter == this->mActionProxyMap.end() || iter->second.empty())
		{
			this->StartPullActionList();
			return false;
		}
		actionProxy = iter->second[0];
		return actionProxy->IsAction() ? true : this->StartConnectToAction(actionProxy);
	}

	bool RemoteActionManager::GetActionProxy(const std::string & action, std::vector<shared_ptr<RemoteActionProxy>> & actionProxys)
	{
		auto iter = this->mActionProxyMap.find(action);
		if (iter == this->mActionProxyMap.end() || iter->second.empty())
		{
			this->StartPullActionList();
			return false;
		}
		for (size_t index = 0; index < iter->second.size(); index++)
		{
			shared_ptr<RemoteActionProxy> actionPrixy = iter->second[index];
			if (!actionPrixy->IsAction() && !this->StartConnectToAction(actionPrixy))
			{
				return false;
			}
		}
		actionProxys = iter->second;
		return true;
	}

	bool RemoteActionManager::GetActionProxy(const std::string & action, long long operId, shared_ptr<RemoteActionProxy> & actionProxy)
	{
		auto iter = this->mActionProxyMap.find(action);
		if (iter == this->mActionProxyMap.end() && iter->second.empty())
		{
			this->StartPullActionList();
			return false;
		}
		//TODO
		return true;
	}

	void RemoteActionManager::GetActionProxyByAddress(const std::string & address, std::vector<shared_ptr<RemoteActionProxy>>& actionProxys)
	{
		for (auto iter = this->mActionProxyMap.begin(); iter != this->mActionProxyMap.end(); iter++)
		{
			for (size_t index = 0; index < iter->second.size(); index++)
			{
				shared_ptr<RemoteActionProxy> actionProxy = iter->second[index];
				if (actionProxy->GetActionAddress() == address)
				{
					actionProxys.push_back(actionProxy);
				}
			}
		}
	}

	void RemoteActionManager::AddNewActionProxy(int argaId, const std::string & name, const std::string & address)
	{
		auto iter = this->mActionProxyMap.find(name);
		if (iter == this->mActionProxyMap.end())
		{
			std::vector<shared_ptr<RemoteActionProxy>> actions;
			this->mActionProxyMap.emplace(name, actions);
		}
		std::vector<shared_ptr<RemoteActionProxy>> & actionList = this->mActionProxyMap[name];
		for (size_t index = 0; index < actionList.size(); index++)
		{
			if (actionList[index]->GetActionAddress() == address)
			{
				return;
			}
		}
		shared_ptr<RemoteActionProxy> actionProxy = make_shared<RemoteActionProxy>(name, address, argaId);
		actionList.push_back(actionProxy);
	}
	void RemoteActionManager::StartRegisterAction()
	{
		PB::ActionUpdateInfo actionInfo;
		RemoteScheduler quertShceudler(this->mActionQuerySession);
		const std::string & address = this->mListenerManager->GetAddress();	
		SayNoDebugWarning("connect address manager success " << mActionQuerySession->GetAddress());
		actionInfo.set_address(address);
		actionInfo.set_areaid(mAreaId);
		std::vector<std::string> actions;
		this->mActionManager->GetAllFunction(actions);

		for (size_t index = 0; index < actions.size(); index++)
		{
			const std::string & actionName = actions[index];
			actionInfo.add_action_names()->assign(actionName);
		}

		quertShceudler.Call("ActionRegisterManager.RegisterActions", &actionInfo, 
			[this](shared_ptr<TcpClientSession> session, XCode code)
		{
			void StartPullActionList();
		});	
	}
	void RemoteActionManager::StartPullActionList()
	{
		Int32Data requestData;
		requestData.set_data(this->mAreaId);
		RemoteScheduler quertShceudler(this->mActionQuerySession);
		quertShceudler.Call<ActionInfoList>("ActionRegisterManager.QueryActions", &requestData,
			[this](shared_ptr<TcpClientSession> session, XCode code, const PB::ActionInfoList & returnData)
		{
			for (int index = 0; index < returnData.actionlist_size(); index++)
			{
				const ActionInfo & actionInfo = returnData.actionlist(index);
				const int actionAreaId = actionInfo.adreid();
				const std::string & actionName = actionInfo.actionname();
				const std::string & actionAddress = actionInfo.address();
				this->AddNewActionProxy(actionAreaId, actionName, actionAddress);
				SayNoDebugWarning(actionAreaId << "\t" << actionAddress << "\t" << actionName);
			}
		});
	}
	bool RemoteActionManager::StartConnectToAction(shared_ptr<RemoteActionProxy> actionProxy)
	{
		shared_ptr<TcpClientSession> tcpClientSession;
		const std::string & address = actionProxy->GetActionAddress();
		auto iter = this->mActionSessionMap.find(address);
		if (iter != this->mActionSessionMap.end())
		{
			tcpClientSession = iter->second;
			if (tcpClientSession->IsActive())
			{
				return true;
			}
		}
		else
		{
			std::string ip;
			unsigned short port;
			if (!StringHelper::ParseIpAddress(address, ip, port))
			{
				return false;
			}
			tcpClientSession = make_shared<TcpClientSession>(this, "ActionSession", ip, port);
			this->mActionSessionMap.emplace(address, tcpClientSession);
		}
		return tcpClientSession->StartConnect();
	}
}