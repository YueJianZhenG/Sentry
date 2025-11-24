#include <sys/stat.h>
#include "FileHelper.h"
#include <fstream>
#ifdef __OS_WIN__
#include <io.h>
#define stat _stat
#else
#include <unistd.h>
#endif

#include <regex>
#include "fmt.h"
#include "DirectoryHelper.h"
#include "Util/Tools/String.h"
#include "Util/Core/Com.h"
#pragma warning(disable : 4996)


namespace help
{
	bool fs::GetFileLine(const std::string& path, size_t& size)
	{
		std::ifstream fs;
		fs.open(path, std::ios::in);
		if (!fs.is_open())
		{
			return false;
		}
		std::string data;
		while (std::getline(fs, data))
		{
			size++;
			data.clear();
		}
		return true;
	}

	bool fs::GetFileSize(const std::string& path, size_t& size)
	{
		struct stat result{};
		if (stat(path.c_str(), &result) != 0)
		{
			return false;
		}
		size = result.st_size;
		return true;
	}

	bool fs::GetFileSize(const std::string& path, std::string& str)
	{
		size_t size = 0;
		if(!help::fs::GetFileSize(path, size))
		{
			return false;
		}
		str = help::com::BytesToString(size);
		return true;
	}

	bool fs::FileIsExist(const std::string & str)
	{
#ifdef _WIN32
		if(_access(str.c_str(), 0) != 0)
		{
			std::string path = help::text::Utf8ToGB2312(str);
			return _access(path.c_str(), 0) == 0;
		}
		return true;
#else
		return access(str.c_str(), F_OK) == 0;
#endif
	}

	long long fs::GetLastWriteTime(const std::string & str)
	{
		struct stat result{};
#ifdef _WIN32
		if (stat(str.c_str(), &result) != 0)
		{
			std::string path = help::text::Utf8ToGB2312(str);
			if (stat(path.c_str(), &result) != 0)
			{
				return 0;
			}
		}
#else
		if (stat(str.c_str(), &result) != 0)
		{
			return 0;
		}
#endif

		return result.st_mtime;
	}

	bool fs::GetFileName(const std::string& path, std::string& name)
	{
		std::regex pattern("/([^/]+)\\.[\\w]+$");
		std::smatch match;
		if (!std::regex_search(path, match, pattern))
		{
			return false;
		}
		name = match[1];
		return true;
	}

	extern bool fs::GetFileType(const std::string& path, std::string& type)
	{
		std::smatch match;
		std::regex pattern("\\.([a-zA-Z0-9]+)$");
		if (std::regex_search(path, match, pattern))
		{
			type = match[1];
			std::transform(type.begin(), type.end(), type.begin(), ::tolower);
			return true;
		}
		return false;
	}

	bool fs::ReadTxtFile(const std::string& str, std::string& outFile)
	{
		std::ifstream fs;
#ifdef __OS_WIN__
		fs.open(str, std::ios::in | std::ios::binary);
		if(!fs.is_open())
		{
			std::string path = help::text::Utf8ToGB2312(str);
			fs.open(path, std::ios::in | std::ios::binary);
		}
#else
		fs.open(str, std::ios::in | std::ios::binary);
#endif
		if(!fs.is_open())
		{
			return false;
		}
		outFile.clear();
		char buffer[128] = { 0 };
		do
		{
			fs.read(buffer, sizeof(buffer));
			size_t count = fs.gcount();
			if (count > 0)
			{
				outFile.append(buffer, count);
			}
		}
		while (!fs.eof());
		fs.close();
		return true;
	}

	bool fs::ReadTxtFile(const std::string& str, std::vector<std::string>& outLines, char delim)
	{
		std::ifstream fs;
#ifdef __OS_WIN__
		fs.open(str, std::ios::in | std::ios::binary);
		if(!fs.is_open())
		{
			std::string path = help::text::Utf8ToGB2312(str);
			fs.open(path, std::ios::in | std::ios::binary);
		}
#else
		fs.open(str, std::ios::in | std::ios::binary);
#endif
		if (fs.is_open())
		{
			std::string tempString;
			while (std::getline(fs, tempString, delim))
			{
				outLines.push_back(tempString);
				tempString = "";
			}
			return true;
		}
		return false;
	}

	bool fs::WriterFile(const std::string& path, const std::string& fileContent)
	{
		std::string nDirector;
		std::string nFileName;
		if (!dir::GetDirAndFileName(path, nDirector, nFileName))
		{
			return false;
		}
		if (!dir::DirectorIsExist(nDirector))
		{
			dir::MakeDir(nDirector);
		}
		std::fstream fs(path, std::ios::ate | std::ios::out | std::ios::binary);
		if (!fs.is_open())
		{
			return false;
		}
		fs.write(fileContent.c_str(), (std::streamsize)fileContent.size());
		fs.close();
		return true;
	}

	bool fs::Open(std::ifstream& fs, const std::string& path, std::ios_base::openmode flag)
	{
		fs.open(path, flag);
		if(!fs.is_open())
		{
#ifdef __OS_WIN__
			std::string str = help::text::Utf8ToGB2312(path);
			fs.open(str, flag);
#endif
		}
		return fs.is_open();
	}

	bool fs::Open(std::ofstream& fs, const std::string& path, std::ios_base::openmode flag)
	{
		fs.open(path, flag);
		if(!fs.is_open())
		{
#ifdef __OS_WIN__
			std::string str = help::text::Utf8ToGB2312(path);
			fs.open(str, flag);
#endif
		}
		return fs.is_open();
	}

	bool fs::ChangeName(const std::string& path, const std::string& name)
	{
		const char * oldFilePath = path.c_str();
		const char* directoryPath = strrchr(oldFilePath, '/');
		if (directoryPath)
		{
			++directoryPath;
			std::string director;
			char newFilePath[256] = { 0 };
			snprintf(newFilePath, sizeof(newFilePath), "%.*s%s", static_cast<int>(directoryPath - oldFilePath),
					oldFilePath, name.c_str());
			if(help::dir::GetDirByPath(newFilePath, director))
			{
				help::dir::MakeDir(director);
			}
			std::remove(newFilePath);
			return std::rename(oldFilePath, newFilePath) == 0;
		}
		return false;
	}
}// namespace FileHelper