//
// Created by zmhy0073 on 2022/10/13.
//
#include"TextConfig.h"
#include"File/FileHelper.h"
#include"Md5/MD5.h"
#include"Log/CommonLogDef.h"
#include"App/System/System.h"
namespace Sentry
{
    const std::string &TextConfig::WorkPath() const
    {
        return System::GetWorkPath();
    }
    bool TextConfig::LoadConfig(const std::string &path)
    {
        std::string content;
        this->mPath = path;
        if(!Helper::File::ReadTxtFile(this->mPath, content))
        {
            CONSOLE_LOG_ERROR("read file [" << path << "] error");
            return false;
        }
        if(this->OnLoadText(content.c_str(), content.size()))
        {
            this->mMd5 = Helper::Md5::GetMd5(content);
            return true;
        }
        return false;
    }

    bool TextConfig::ReloadConfig()
    {
        std::string content;
        if(!Helper::File::ReadTxtFile(this->mPath, content))
        {
            CONSOLE_LOG_ERROR("read file [" << this->mPath << "] error");
            return false;
        }

        std::string md5 = Helper::Md5::GetMd5(content);
        if(md5 == this->mMd5)
        {
            return true;
        }
        if(this->OnReloadText(content.c_str(), content.size()))
        {
            this->mMd5 = md5;
            return true;
        }
        return false;
    }
}