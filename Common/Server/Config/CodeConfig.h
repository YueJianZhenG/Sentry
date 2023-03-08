//
// Created by zmhy0073 on 2022/10/19.
//

#pragma once
#include<memory>
#include<unordered_map>
#include"XCode/XCode.h"
#include"Config/TextConfig.h"
#include"Singleton/Singleton.h"
namespace Sentry
{
    class CodeLineConfig
    {
    public:
        int Code;
        std::string Name;
        std::string Desc;
    };

    class CodeConfig : public TextConfig, public ConstSingleton<CodeConfig>
    {
    public:
        CodeConfig() : TextConfig("CodeConfig") { }
    public:
        const std::string & GetDesc(int code) const;
    private:
        bool OnLoadText(const char *str, size_t length) final;
        bool OnReloadText(const char *str, size_t length) final;
    private:
        std::unordered_map<int, std::unique_ptr<CodeLineConfig>> mConfigs;
    };
}
