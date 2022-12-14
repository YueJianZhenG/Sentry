//
// Created by zmhy0073 on 2022/6/21.
//

#ifndef SERVER_HTTPSERVICECOMPONENT_H
#define SERVER_HTTPSERVICECOMPONENT_H
#include"HttpListenComponent.h"
#include"Component/RpcTaskComponent.h"
namespace Http
{
    class Request;
    class Response;
}
namespace Sentry
{
    class HttpMethodConfig;
    class HttpHandlerClient;
    class HttpWebComponent : public HttpListenComponent, public IServerRecord
    {
    public:
        HttpWebComponent() = default;
        ~HttpWebComponent() = default;
    public:
        unsigned int GetWaitCount() const { return this->mWaitCount; }
    private:
        bool LateAwake() final;
        void OnRecord(Json::Writer &document) final;
        bool OnDelClient(const std::string& address);
		void OnRequest(const std::string &address, std::shared_ptr<Http::Request> request) final;
		void Invoke(const std::string& address, const HttpMethodConfig* config, std::shared_ptr<Http::Request> request);
    private:
        unsigned int mSumCount;
        unsigned int mWaitCount;
        TaskComponent * mTaskComponent;
        std::unordered_map<std::string, unsigned int> mTasks;
    };
}


#endif //SERVER_HTTPSERVICECOMPONENT_H
