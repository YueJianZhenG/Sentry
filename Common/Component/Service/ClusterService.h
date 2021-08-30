﻿#pragma once

#include<Service/LocalService.h>
using namespace com;
namespace Sentry
{
    class ServiceMgrComponent;

    class CoroutineComponent;

    class ServiceNodeComponent;

    class ClusterService : public LocalService
    {
    public:
        ClusterService() {}

        ~ClusterService() {}

    public:
        bool Awake() final;

        void Start() final;

		int GetPriority() final { return 1000; }
    private:
        XCode DelNode(long long, const Int32Data &node);

        XCode AddNode(long long, const s2s::NodeData_NodeInfo &nodeInfo);

    private:
        short mAreaId;
        short mNodeId;
        std::string mListenAddress;
        ServiceNodeComponent * mNodeComponent;
    };
}