//
// Created by 64658 on 2025/6/21.
//

#include "FileFactory.h"
#include "FileHelper.h"
namespace help
{
	bool FileFactory::IsChange(const std::string& path)
	{
		auto iter = this->mFileWriteTimes.find(path);
		if(iter == this->mFileWriteTimes.end())
		{
			return true;
		}
		long long lastWrite = help::fs::GetLastWriteTime(path.c_str());
		return lastWrite != iter->second;
	}

	bool FileFactory::Read(const std::string& path, std::string& content)
	{
		if(!help::fs::ReadTxtFile(path, content))
		{
			return false;
		}
		this->mFileWriteTimes[path] = help::fs::GetLastWriteTime(path.c_str());
		return true;
	}

	bool FileFactory::Read(const std::string& path, json::r::Document& document)
	{
		if(!document.FromFile(path))
		{
			return false;
		}
		this->mFileWriteTimes[path] = help::fs::GetLastWriteTime(path.c_str());
		return true;
	}

	bool FileFactory::Read(const std::string& path, std::vector<std::string>& content)
	{
		if(!help::fs::ReadTxtFile(path, content))
		{
			return false;
		}
		this->mFileWriteTimes[path] = help::fs::GetLastWriteTime(path.c_str());
		return true;
	}

	bool FileFactory::Read(const std::string& path, json::IObject& document)
	{
		json::r::Document document1;
		if(!document1.FromFile(path))
		{
			return false;
		}
		if(!document.Decode(document1))
		{
			return false;
		}
		this->mFileWriteTimes[path] = help::fs::GetLastWriteTime(path.c_str());
		return true;
	}

	bool FileFactory::Read(const std::string& path, json::r::Document& document, json::IObject& obj)
	{
		if(!document.FromFile(path))
		{
			return false;
		}
		if(!obj.Decode(document))
		{
			return false;
		}
		this->mFileWriteTimes[path] = help::fs::GetLastWriteTime(path.c_str());
		return true;
	}
}