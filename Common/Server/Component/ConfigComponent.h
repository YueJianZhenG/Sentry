//
// Created by zmhy0073 on 2022/10/13.
//

#ifndef APP_CONFIGCOMPONENT_H
#define APP_CONFIGCOMPONENT_H
#include<unordered_map>
#include"Config/Base/TextConfig.h"
#include"Entity/Component/Component.h"
namespace acs
{
	class ConfigComponent final : public Component, public IRefresh, public IDestroy
    {
    public:
        ConfigComponent() = default;
        ~ConfigComponent() override = default;
    public:
        bool OnRefresh() final;

        template<typename T>
        const T * GetTextConfig();

        template<typename T>
        const T * GetTextConfig(const std::string & name);

        template<typename T>
        bool LoadTextConfig(const std::string & path);
        bool LoadTextConfig(std::unique_ptr<ITextConfig> config, const std::string & path);
    private:
        bool Awake() final;
		void OnDestroy() final;
		bool LoadInterfaceConfig();
    private:
        std::unordered_map<size_t, std::string> mKeys;
        std::unordered_map<std::string, std::unique_ptr<ITextConfig>> mConfigs;
    };

    template<typename T>
    bool ConfigComponent::LoadTextConfig(const std::string & path)
    {
        std::unique_ptr<T> config(new T());
        size_t key = typeid(T).hash_code();
        if(this->mKeys.find(key) != this->mKeys.end())
        {
            LOG_ERROR("multiple load {} type ={}", config->GetConfigName(), typeid(T).name());
            return false;
        }
        this->mKeys[key] = config->GetConfigName();
        return this->LoadTextConfig(std::move(config), path);
    }
    template<typename T>
    const T * ConfigComponent::GetTextConfig()
    {
        size_t key = typeid(T).hash_code();
        auto iter = this->mKeys.find(key);
        if(iter == this->mKeys.end())
        {
            return nullptr;
        }
        auto iter1 = this->mConfigs.find(iter->second);
        return iter1 != this->mConfigs.end() ?  static_cast<T*>(iter1->second.get()) : nullptr;
    }

    template<typename T>
    const T *ConfigComponent::GetTextConfig(const std::string &name)
    {
        auto iter1 = this->mConfigs.find(name);
        return iter1 != this->mConfigs.end() ?  static_cast<T*>(iter1->second.get()) : nullptr;
    }
}


#endif //APP_CONFIGCOMPONENT_H
