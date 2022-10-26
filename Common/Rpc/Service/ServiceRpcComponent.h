//
// Created by yjz on 2022/6/5.
//

#ifndef _SERVICEAGENT_H_
#define _SERVICEAGENT_H_
#include"RpcService.h"
namespace Sentry
{
	class ServiceRpcComponent : public RpcService
	{
	 public:
		ServiceRpcComponent() = default;
		~ServiceRpcComponent() = default;
	 private:
		bool Start() final { return false;}
		bool Close() final { return false;}
		void WaitAllMessageComplete() final { };
		bool IsStartService() final { return false; }
    private:
		void GetSubEventIds(std::unordered_set<std::string> &evendIds) final { };
		XCode Invoke(const std::string &id, const std::string &message) final;
		XCode Invoke(const std::string& name, std::shared_ptr<Rpc::Packet> message) final;
	};
}

#endif //_SERVICEAGENT_H_