//
// Created by zmhy0073 on 2022/10/25.
//

#ifndef APP_LOCATIONSERVICE_H
#define APP_LOCATIONSERVICE_H
#include"Message/s2s.pb.h"
#include"Service/PhysicalService.h"
namespace Sentry
{
	class Registry final : public PhysicalService, public ISecondUpdate
    {
    public:
        Registry() = default;
    public:
        bool OnStart() final;
    private:
        int Ping(const Rpc::Packet & head);
        int UnRegister(const com::type::string& request);
        int Query(const com::array::string& request, s2s::server::list& response);
		int Register(const s2s::server::info & request, s2s::server::list & response);
    private:
		void OnSecondUpdate(int tick) final;
        void OnNodeServerError(const std::string& address);
	 private:
        class NodeMgrComponent * mNodeComponent;
        class InnerNetComponent* mInnerComponent;
		std::unordered_map<std::string, long long> mPingInfos;
    };
}


#endif //APP_LOCATIONSERVICE_H
