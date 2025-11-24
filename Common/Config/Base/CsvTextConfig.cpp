//
// Created by yjz on 2023/3/23.
//
#include<sstream>
#include<fstream>
#include"CsvTextConfig.h"
#include"Util/Tools/String.h"
#include"Util/File/FileHelper.h"
namespace acs
{
	bool CsvLineData::Add(const std::string& key, const std::string& value)
	{
		auto iter = this->mDatas.find(key);
		if(iter != this->mDatas.end())
		{
			return false;
		}
		this->mDatas.emplace(key, value);
		return true;
	}

	bool CsvLineData::Get(const std::string& key, int& value) const
	{
		auto iter = this->mDatas.find(key);
		if(iter == this->mDatas.end())
		{
			return false;
		}
		value = std::stoi(iter->second);
		return true;
	}

	bool CsvLineData::Get(const std::string& key, bool& value) const
	{
		auto iter = this->mDatas.find(key);
		if(iter == this->mDatas.end())
		{
			return false;
		}
		value = iter->second == "true";
		return true;
	}

	bool CsvLineData::Get(const std::string& key, long long& value) const
	{
		auto iter = this->mDatas.find(key);
		if(iter == this->mDatas.end())
		{
			return false;
		}
		value = std::stoll(iter->second);
		return true;
	}

	bool CsvLineData::Get(const std::string& key, std::string& value) const
	{
		auto iter = this->mDatas.find(key);
		if(iter == this->mDatas.end())
		{
			return false;
		}
		value = iter->second;
		return true;
	}
}

namespace acs
{
	CsvTextConfig::CsvTextConfig(std::string  name)
		: mName(std::move(name)), mLastWriteTime(0)
	{

	}

	bool CsvTextConfig::ReloadConfig()
	{
		long long writeTime =help::fs::GetLastWriteTime(this->mPath.c_str());
		if(writeTime == this->mLastWriteTime)
		{
			return true;
		}
		this->mLastWriteTime = writeTime;
		std::ifstream fileStream(this->mPath, std::ios::out);
		if(!fileStream.is_open())
		{
			return false;
		}

		std::string line;
		if(!std::getline(fileStream, line))
		{
			return false;
		}
		std::vector<std::string> fields;
		std::vector<std::string> values;
		if(help::Str::Split(line, ',', fields) == 0)
		{
			return false;
		}
		size_t size = fields.size();
		while(std::getline(fileStream, line))
		{
			if(line[0] == '#') //忽略注释行
			{
				continue;
			}
			values.clear();
			CsvLineData lineData;
			if(help::Str::Split(line, ',', values) != size)
			{
				return false;
			}
			for(size_t index = 0;index<values.size();index++)
			{
				const std::string & key = fields.at(index);
				const std::string & value = values.at(index);
				lineData.Add(key, value);
			}
			if(!this->OnReLoadLine(lineData))
			{
				return false;
			}
		}
		return true;
	}

	bool CsvTextConfig::LoadConfig(const std::string& path)
	{
		std::ifstream fileStream(path, std::ios::out);
		if(!fileStream.is_open())
		{
			return false;
		}
		std::string line;
		if(!std::getline(fileStream, line))
		{
			return false;
		}
		if(line.back() == '\r')
		{
			line.pop_back();
		}
		std::vector<std::string> fields;
		std::vector<std::string> values;
		if(help::Str::Split(line, ',', fields) == 0)
		{
			return false;
		}
		size_t size = fields.size();
		while(std::getline(fileStream, line))
		{
			if(line[0] == '#') //忽略注释行
			{
				continue;
			}
			values.clear();
			if(line.back() == '\r')
			{
				line.pop_back();
			}
			CsvLineData lineData;
			if(help::Str::Split(line, ',', values) != size)
			{
				return false;
			}
			for(size_t index = 0;index<values.size();index++)
			{
				const std::string & key = fields.at(index);
				const std::string & value = values.at(index);
				lineData.Add(key, value);
			}
			if(!this->OnLoadLine(lineData))
			{
				return false;
			}
		}
		this->mPath = path;
		this->mLastWriteTime = help::fs::GetLastWriteTime(path.c_str());
		return true;
	}
}