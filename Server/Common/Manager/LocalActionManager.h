#pragma once
#include<Manager/Manager.h>
#include<Other/TimeRecorder.h>

namespace SoEasy
{
	// ע�᱾�ط��� ���ͱ��ط���ָ����ַ ����Զ�̻����Ļص�
	class NetLuaAction;
	class LocalActionProxy;
	class LocalRetActionProxy;

	class LocalActionManager : public Manager
	{
	public:
		LocalActionManager();
		virtual ~LocalActionManager() { }
	public:
		bool BindFunction(shared_ptr<NetLuaAction> actionBox);
		bool BindFunction(shared_ptr<LocalActionProxy> actionBox);
		void GetAllFunction(std::vector<std::string> & funcs);
	public:
		bool DelCallback(long long callbackId);
		shared_ptr<LocalRetActionProxy> GetCallback(long long callbackId, bool remove = true);
		bool AddCallback(shared_ptr<LocalRetActionProxy> actionBox, long long & callbackId);
	protected:
		bool OnInit() override;
		void OnDestory() override;
		void OnSecondUpdate() override;
		void OnInitComplete() override;
	/*public:
		bool Call(shared_ptr<TcpClientSession> tcpSession, const long long id, const shared_ptr<NetWorkPacket> callInfo);
		bool Call(shared_ptr<TcpClientSession> tcpSession, const std::string & name, const shared_ptr<NetWorkPacket> callInfo);*/
	public:		
		shared_ptr<NetLuaAction> GetLuaAction(const std::string & name);
		shared_ptr<LocalActionProxy> GetAction(const std::string & name);
	private:
		std::string mMessageBuffer;
		TimeRecorder mLogicTimeRecorder;
		TimeRecorder mNetLatencyRecorder;
		class ScriptManager * mScriptManager;
		class NetWorkManager * mNetWorkManager;
		class CoroutineManager * mCoroutineScheduler;
	private:
		std::unordered_map<long long, shared_ptr<LocalRetActionProxy>> mRetActionMap;
		std::unordered_map<std::string, shared_ptr<NetLuaAction>> mRegisterLuaActions;
		std::unordered_map<std::string, shared_ptr<LocalActionProxy>> mRegisterActions;
	};
}