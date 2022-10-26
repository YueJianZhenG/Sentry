//
// Created by zmhy0073 on 2022/10/17.
//

#include"ClusterConfig.h"
#include"App/System/System.h"
namespace Sentry
{
    bool NodeConfig::IsStart(const std::string &service) const
    {
        auto iter = this->mServices.find(service);
        return iter != this->mServices.end() && iter->second;
    }

    bool NodeConfig::OnLoadConfig(const rapidjson::Value &value)
    {
        if(value.HasMember("Service"))
        {
            if(!value["Service"].IsObject())
            {
                return false;
            }
            auto iter = value["Service"].MemberBegin();
            for(; iter != value["Service"].MemberEnd(); iter++)
            {
                const std::string service(iter->name.GetString());
                this->mServices.emplace(service, iter->value.GetBool());
            }
        }
        if(value.HasMember("Component"))
        {
            if(!value["Component"].IsArray())
            {
                return false;
            }
            for(size_t index = 0; index < value["Component"].Size(); index++)
            {
                const rapidjson::Value & data = value["Component"][index];
                this->mComponents.emplace(std::string(data.GetString()));
            }
        }
        if(value.HasMember("AutoAllot"))
        {
            this->mIsAutoAllot = value["AutoAllot"].GetBool();
        }
        if(value.HasMember("lua"))
        {
            this->mLua = value["lua"].GetString();
        }
        return true;
    }

    size_t NodeConfig::GetComponents(std::vector<std::string> &components) const
    {
        for(const std::string & name : this->mComponents)
        {
            components.emplace_back(name);
        }
        return components.size();
    }

    bool NodeConfig::HasService(const std::string &service) const
    {
        auto iter = this->mServices.find(service);
        return iter != this->mServices.end() && iter->second;
    }

    size_t NodeConfig::GetServices(std::vector<std::string> &services, bool start) const
    {
        for(auto & value : this->mServices)
        {
            if(start)
            {
                if(value.second)
                {
                    services.emplace_back(value.first);
                }
                continue;
            }
            services.emplace_back(value.first);
        }
        return services.size();
    }

}

namespace Sentry
{
    bool ClusterConfig::OnLoadText(const char *str, size_t length)
    {
        rapidjson::Document document;
        if(document.Parse(str, length).HasParseError())
        {
            return false;
        }
        auto iter = document.MemberBegin();
        for(; iter != document.MemberEnd(); iter++)
        {
            const std::string name(iter->name.GetString());
            const rapidjson::Value & jsonValue = iter->value;
            std::unique_ptr<NodeConfig> nodeConfig(new NodeConfig(name));
            if(!nodeConfig->OnLoadConfig(jsonValue))
            {
                return false;
            }
            this->mNodeConfigs.emplace(name, std::move(nodeConfig));
        }
        return true;
    }

    bool ClusterConfig::OnReloadText(const char *str, size_t length)
    {
        return true;
    }

    const NodeConfig *ClusterConfig::GetConfig() const
    {
        const std::string & name = System::GetName();
        auto iter = this->mNodeConfigs.find(name);
        return iter != this->mNodeConfigs.end() ? iter->second.get() : nullptr;
    }

    const NodeConfig *ClusterConfig::GetConfig(const std::string &name) const
    {
        auto iter = this->mNodeConfigs.find(name);
        return iter != this->mNodeConfigs.end() ? iter->second.get() : nullptr;
    }

    size_t ClusterConfig::GetNodeConfigs(std::vector<const NodeConfig *> &nodes) const
    {
        auto iter = this->mNodeConfigs.begin();
        for(; iter != this->mNodeConfigs.end(); iter++)
        {
            nodes.emplace_back(iter->second.get());
        }
        return nodes.size();
    }
}