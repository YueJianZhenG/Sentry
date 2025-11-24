//
// Created by leyi on 2023/11/20.
//
#include "ResourceMgr.h"
#include "Core/System/System.h"
#include "Entity/Actor/App.h"
#include "Util/Tools/String.h"
#include "Util/File/FileHelper.h"
#include "Util/File/DirectoryHelper.h"
#include "Http/Component/HttpWebComponent.h"
#include "Http/Component/HttpComponent.h"
namespace acs
{
	bool ResourceMgr::Awake()
	{
		const ServerConfig & config = this->mApp->Config();
		{
			json::r::Value jsonObject;
			LOG_CHECK_RET_FALSE(config.Get("http", jsonObject))
			LOG_CHECK_RET_FALSE(config.GetPath("res", this->mUpload));
			LOG_CHECK_RET_FALSE(jsonObject.Get("domain", this->mDoMain))
		}
		LOG_CHECK_RET_FALSE(help::dir::MakeDir(this->mUpload))
		return true;
	}

	bool ResourceMgr::OnInit()
	{
		BIND_COMMON_HTTP_METHOD(ResourceMgr::List);
		BIND_COMMON_HTTP_METHOD(ResourceMgr::Look);
		BIND_COMMON_HTTP_METHOD(ResourceMgr::Video);
		BIND_COMMON_HTTP_METHOD(ResourceMgr::Upload);
		BIND_COMMON_HTTP_METHOD(ResourceMgr::Download);
		this->GetComponent<HttpWebComponent>()->AddRootDirector(this->mUpload);
		return true;
	}

    int ResourceMgr::Download(const http::FromContent &request, http::Response &response)
    {
		std::string file;
        if(!request.Get("file", file))
        {
            return XCode::CallArgsError;
        }
        std::string fileName;
        std::string path = fmt::format("{}/{}", this->mUpload, file);
        {
            size_t pos = path.find_last_of('/');
            if(pos != std::string::npos)
            {
                fileName = path.substr(pos + 1);
            }
        }
        response.Header().Add("Content-Disposition", fmt::format("attachment; filename={}", fileName));
        response.File(http::Header::Bin, path);
        return XCode::Ok;
    }

	int ResourceMgr::Look(const http::FromContent& request, http::Response& response)
	{
		std::string file, type;
		if(!request.Get("file", file) || file.empty())
		{
			response.SetCode(HttpStatus::BAD_REQUEST);
			return XCode::CallArgsError;
		}
		std::string path;
		do
		{
			path = fmt::format("{}/{}", this->mUpload, file);
			if(help::fs::FileIsExist(path.c_str()))
			{
				break;
			}
			path = fmt::format("{}{}", this->mUpload, file);
		}
		while(false);
		LOG_INFO("path = {}", path);
		if(!help::fs::GetFileType(path, type))
		{
			response.SetCode(HttpStatus::INTERNAL_SERVER_ERROR);
			return XCode::Failure;
		}
		response.File(http::GetContentType(type), path);
		return XCode::Ok;
	}

	int ResourceMgr::List(const http::FromContent & request, http::Response& response)
	{
		std::string dir;
		request.Get("dir", dir);
		dir = dir.empty() ? this->mUpload : fmt::format("{}/{}", this->mUpload, dir);
		std::unique_ptr<http::TextContent> customData(new http::TextContent(http::Header::HTML));
		{
			LOG_WARN("dir = {}", dir);
			customData->Append("<!DOCTYPE html>");
			customData->Append("<meta http-equiv='Content-Type' content='text/html;charset=utf-8'/>");
			customData->Append("<html>");
			customData->Append("<head>");
			customData->Append(fmt::format("<title>{}</title>", "资源列表"));
			customData->Append("/<head>");
			customData->Append("<body>");
			customData->Append("<li>");
			size_t index = 1;
			std::vector<std::string> directors, files;
			if(dir != this->mUpload)
			{
				index = 0;
				directors.emplace_back(dir);
				while(directors.front().back() != '/')
				{
					directors.front().pop_back();
				}
				directors.front().pop_back();
				customData->Append(fmt::format("<a href='/res/list'>{}</a>", "root"));
				customData->Append("</li>");
			}
			help::dir::GetDirAndFiles(dir, directors, files);
			for(const std::string & fullDir : directors)
			{
				std::string key, director;
				customData->Append("<li>");
				if(index == 0)
				{
					key = "返回";
					index++;
				}
				else
				{
					key = fullDir.substr(dir.size() + 1);
				}
				if(fullDir.size() > this->mUpload.size())
				{
					director = fullDir.substr(this->mUpload.size() + 1);
				}
				//director = fullDir.substr(this->mDownload.size() + 1);
				std::string url = fmt::format("/res/list?dir={}", director);
				customData->Append(fmt::format("<a href='{}'>{}</a>", url, key));
				customData->Append("</li>");
			}
			std::sort(files.begin(), files.end(),
					[](const std::string & str1, const std::string & str2) ->bool {
						return str1.size() < str2.size();
			});
			int count = 1;
			for (const std::string& fullPath: files)
			{
				std::string suffix, fileSize;
				help::fs::GetFileType(fullPath, suffix);
				std::string key = fullPath.substr(dir.size() + 1);
				std::string path = fullPath.substr(this->mUpload.size());
				{
					std::string url = fmt::format("/res/look?file={}", path);
					std::string download = fmt::format("/res/download?file={}", path);
					customData->Append("<li>");
					help::fs::GetFileSize(fullPath, fileSize);
					customData->Append(fmt::format("<span>({}) </span>", count++));
					customData->Append(fmt::format("<span>[{}] &nbsp </span>", fileSize));
					//customData->Append(fmt::format("<a href='{}'>[{}]</a>", download, "下载"));
					customData->Append(fmt::format("<a href='{}'>[{}]</a>", url, key));
					customData->Append("</li>");
					//html.emplace_back(fmt::format("<li><a href='{}'>{}</a></li>", url, path.substr(1)));
				}
			}
			customData->Append("</li>");
			customData->Append("</body>");
			customData->Append("</html>");
		}
		response.SetContent(customData);
		return XCode::Ok;
	}

	int ResourceMgr::Upload(const http::Request& request, http::Response& response)
	{
		int userId = 0;
		const http::Content* data = request.GetBody();
		request.GetUrl().GetQuery().Get(http::query::UserId, userId);
		const http::MultipartFromContent* multiData = data->To<const http::MultipartFromContent>();
		if (multiData == nullptr)
		{
			return XCode::CallArgsError;
		}
		if (!multiData->IsDone())
		{
			return XCode::CallArgsError;
		}
		const std::string & path = multiData->Path();
		const std::string& name = multiData->FileName();
		const std::string url = fmt::format("{}/{}", this->mDoMain, name);
		response.SetContent(http::Header::TEXT, url);
		return XCode::Ok;
	}

	int ResourceMgr::Video(const http::FromContent& request, json::w::Document& response)
	{
		HttpComponent* httpComponent = this->GetComponent<HttpComponent>();
		std::unique_ptr<http::Response> response1 = httpComponent->Get("http://api.yujn.cn/api/zzxjj.php?type=video");
		if(response1 == nullptr)
		{
			return XCode::Failure;
		}
		std::string url;
		if(!response1->Header().Get("Location", url))
		{
			return XCode::Failure;
		}
		response.Add("url", url);
		return XCode::Ok;
	}
}