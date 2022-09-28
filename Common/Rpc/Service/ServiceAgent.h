//
// Created by yjz on 2022/6/5.
//

#ifndef _SERVICEAGENT_H_
#define _SERVICEAGENT_H_
#include"Service.h"
namespace Sentry
{
	class ServiceAgent : public Service
	{
	 public:
		ServiceAgent() = default;
		~ServiceAgent() = default;
	 private:
		bool StartNewService() final { return false;}
		bool CloseService() final { return false;}
		bool IsStartService() final { return false; }
	 private:
		XCode Invoke(const std::string& name, std::shared_ptr<Rpc::Data> message) final;
	};
}

#endif //_SERVICEAGENT_H_
