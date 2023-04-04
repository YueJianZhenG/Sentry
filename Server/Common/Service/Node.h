//
// Created by zmhy0073 on 2022/10/8.
//

#ifndef APP_INNERSERVICE_H
#define APP_INNERSERVICE_H

#include"Message/s2s.pb.h"
#include"Message/com.pb.h"
#include"Rpc/Service/PhysicalRpcService.h"
namespace Tendo
{
	class Node final : public PhysicalRpcService, public IComplete
    {
    public:
        Node();
        ~Node() = default;
    private:
        int Stop();
        int Hotfix();
        int LoadConfig();
		int Ping(const Rpc::Packet& packet);
		int Join(const s2s::server::info& request); //新服务器加入
        int Exit(const com::type::string& request); //服务器退出
        int RunInfo(com::type::string& response); // 获取运行信息
    private:
		bool Awake() final;
		bool OnInit() final;
		void OnClose() final;
		void OnLocalComplete() final;
    private:
        class NodeMgrComponent* mNodeComponent;
    };
}


#endif //APP_INNERSERVICE_H
