//
// Created by zmhy0073 on 2022/10/19.
//

#include"CodeConfig.h"
#include"String/StringHelper.h"
namespace Sentry
{
    bool CodeConfig::OnLoadText(const std::string &content)
    {
        std::vector<std::string> lines;
        if(Helper::String::Split(content, "\n", lines) <= 0)
        {
            return false;
        }
        std::vector<std::string> rets;
        for(size_t index = 0; index < lines.size() -1; index++)
        {
            rets.clear();
            if(Helper::String::Split(lines[index + 1], "\t", rets) != 2)
            {
                return false;
            }
            std::unique_ptr<CodeLineConfig> lineConfig(new CodeLineConfig());
            {
                lineConfig->Name = rets[0];
                lineConfig->Desc = rets[1];
                lineConfig->Code = (XCode)index;
            }
            this->mConfigs.emplace((int)lineConfig->Code, std::move(lineConfig));
        }
        return true;
    }

    std::string CodeConfig::GetDesc(XCode code) const
    {
        auto iter = this->mConfigs.find((int)code);
        if(iter != this->mConfigs.end())
        {
            return iter->second->Desc;
        }
        return std::string();
    }

    bool CodeConfig::OnReloadText(const std::string &content)
    {
        return true;
    }
}