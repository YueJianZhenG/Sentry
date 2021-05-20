#pragma once
#include<Manager/Manager.h>
#include<NetWork/NetWorkAction.h>
namespace SoEasy
{
	
	
	

	class ServiceBase : public Manager
	{
	public:
		ServiceBase();
		virtual ~ServiceBase() { }
	public:
		void AddActionArgv(SharedPacket argv);
		void AddActionArgv(SharedNetPacket argv);
	protected:
		virtual bool OnInit() override;
		void OnSystemUpdate() final;
	protected:
		bool BindFunction(std::string name, LocalAction1 action);

		template<typename T1>
		bool BindFunction(std::string name, LocalAction2<T1> action);

		template<typename T1, typename T2>
		bool BindFunction(std::string name, LocalAction3<T1, T2> action);
	private:
		XCode CallAction(SharedPacket request, SharedPacket returnData);
		bool BindFunction(const std::string & name, shared_ptr<LocalActionProxy> actionBox);
	private:
		class ActionManager * mActionManager;
		class NetWorkManager * mNetWorkManager;
		class CoroutineManager * mCoroutineManager;
		std::queue<SharedPacket> mLocalMessageQueue;
		DoubleBufferQueue<SharedNetPacket> mHandleMessageQueue;
		std::unordered_map<std::string, shared_ptr<LocalActionProxy>> mActionMap;
	};

	template<typename T1>
	inline bool ServiceBase::BindFunction(std::string name, LocalAction2<T1> action)
	{
		typedef LocalActionProxy2<T1> ActionProxyType;
		return this->BindFunction(name, make_shared<ActionProxyType>(action, name));
	}

	template<typename T1, typename T2>
	inline bool ServiceBase::BindFunction(std::string name, LocalAction3<T1, T2> action)
	{
		const size_t pos = name.find_first_of(".");
		return this->BindFunction(name, make_shared<LocalActionProxy3<T1, T2>>(action, name));
	}
#define REGISTER_FUNCTION_0(func) this->BindFunction(GetFunctionName(#func), std::bind(&func, this, args1))
#define REGISTER_FUNCTION_1(func,t1) this->BindFunction<t1>(GetFunctionName(#func), std::bind(&func, this, args1, args2))
#define REGISTER_FUNCTION_2(func,t1, t2) this->BindFunction<t1, t2>(GetFunctionName(#func), std::bind(&func, this, args1, args2, args3))

}