//
// Created by yjz on 2022/6/5.
//

#ifndef _SERVICEAGENT_H_
#define _SERVICEAGENT_H_
#include"RpcService.h"
namespace Tendo
{
	class VirtualRpcService final : public RpcService
	{
	 public:
		VirtualRpcService() = default;
		~VirtualRpcService() = default;
	 private:
		bool Init() final { return true; }
		bool Start() final { return false;}
		bool Close() final { return false;}
		bool LoadFromLua() final { return false; }
		void WaitAllMessageComplete() final { };
		bool IsStartService() final { return false; }
    private:
		int Invoke(const std::string& name, std::shared_ptr<Msg::Packet> message) final;
	};
}

#endif //_SERVICEAGENT_H_
