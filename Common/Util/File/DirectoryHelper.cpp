#include "DirectoryHelper.h"

#ifdef __OS_WIN__

#include <codecvt>
#include <direct.h>
#include <io.h>
#include <windows.h>

#else
#include <dirent.h>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef __OS_WIN__

#include <codecvt>

#define MakeDirectory(path) _mkdir(path.c_str())
#else
#define MakeDirectory(path) mkdir((path).c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH)
#endif
#define PATH_MAX_LENGHT 1024

#include<regex>

#include <bundled/format.h>
#include "Util/Tools/String.h"

namespace help
{
	bool dir::IsDir(const std::string& str)
	{
#ifdef __OS_WIN__
		std::string dir = help::text::Utf8ToGB2312(str);
		DWORD fileAttributes = GetFileAttributes(dir.c_str());
		if (fileAttributes == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}
		if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			return true;
		}
		return false;
#else
		struct stat statBuf;

		if (stat(str.c_str(), &statBuf) != 0)
		{
			return false;
		}

		if (S_ISREG(statBuf.st_mode))
		{
			return false;
		}
		else if (S_ISDIR(statBuf.st_mode))
		{
			return true;
		}
		return false;
#endif
	}

	bool dir::MakeDir(const std::string& str)
	{
		if (dir::DirectorIsExist(str))
		{
			return true;
		}
#ifdef __OS_WIN__
		std::string dir = help::text::Utf8ToGB2312(str);
#else
		std::string dir = str;
#endif
		for (size_t index = 0; index < dir.size(); index++)
		{
			if (dir[index] == '/' || dir[index] == '\\')
			{
				const std::string path = dir.substr(0, index);
#ifdef __OS_WIN__
				_mkdir(path.c_str());
#else
				MakeDirectory(path);
#endif
			}
		}
#ifdef __OS_WIN__
		return _mkdir(dir.c_str()) != -1;
#else
		return MakeDirectory(dir) != -1;
#endif
	}

	bool dir::IsValidPath(const std::string& str)
	{
#ifdef __OS_WIN__
		std::string path = help::text::Utf8ToGB2312(str);
		const std::regex pathRegex(R"(^([a-zA-Z]:)?[\\/](?:[^\\/:\*\?"<>\|]+[\\/])*[^\\/:\*\?"<>\|]*$)");
		return std::regex_match(path, pathRegex);
#else
		const std::regex pathRegex(R"(^([a-zA-Z]:)?[\\/](?:[^\\/:\*\?"<>\|]+[\\/])*[^\\/:\*\?"<>\|]*$)");
		return std::regex_match(str, pathRegex);
#endif

	}

	bool dir::DeleteDir(const std::string& str)
	{
#ifdef __OS_WIN__
		if(_rmdir(str.c_str()) == -1)
		{
			std::string dir = help::text::Utf8ToGB2312(str);
			return _rmdir(dir.c_str()) != -1;
		}
		return true;
#else
		return rmdir(str.c_str()) != -1;
#endif
	}

	int dir::RemoveAllFile(const std::string& dir)
	{
		int count = 0;
		std::vector<std::string> filePaths;
		dir::GetFilePaths(dir, filePaths);
		for (const std::string& path: filePaths)
		{
			if (std::remove(path.c_str()) != 0)
			{
#ifdef __OS_WIN__
				std::string str = help::text::Utf8ToGB2312(path);
				if(std::remove(str.c_str()) != 0)
				{
					continue;
				}
#else
				continue;;
#endif

			}
			count++;
		}
		dir::DeleteDir(dir);
		return count;
	}

	bool dir::DirectorIsExist(const std::string& str)
	{
#ifdef __OS_WIN__
		if(_access(str.c_str(), 0) != 0)
		{
			std::string dir = help::text::Utf8ToGB2312(str);
			return _access(dir.c_str(), 0) == 0;
		}
		return true;
#else
		return access(str.c_str(), F_OK) == 0;
#endif
	}

	bool dir::GetDirAndFiles(const std::string& path,
			std::vector<std::string>& directorys, std::vector<std::string>& files)
	{
#ifdef __OS_WIN__
		WIN32_FIND_DATA findFileData;
		std::string tempStr = path + "/*.*";
		HANDLE fileHandle = FindFirstFile(tempStr.c_str(), &findFileData);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			std::string temPath = help::text::Utf8ToGB2312(tempStr);
			fileHandle = FindFirstFile(temPath.c_str(), &findFileData);
			if(fileHandle == INVALID_HANDLE_VALUE)
			{
				return false;
			}
		}
		while (true)
		{
			if (findFileData.cFileName[0] != '.')
			{
				std::string name = help::text::GB2312ToUtf8(findFileData.cFileName);
				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					directorys.emplace_back(fmt::format("{}/{}", path, name));
				}
				else
				{
					files.emplace_back(fmt::format("{}/{}", path, name));
				}
			}
			//如果是当前路径或者父目录的快捷方式，或者是普通目录，则寻找下一个目录或者文件
			if (!FindNextFile(fileHandle, &findFileData))
			{
				break;
			}
		}
		FindClose(fileHandle);
#else
		DIR* dir = opendir(path.c_str());
		if (dir == NULL)
		{
			return false;
		}
		while (true)
		{
			struct dirent* ptr = readdir(dir);
			if (ptr == NULL)
			{
				break;
			}
			if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			{
				continue;
			}
			else if (ptr->d_type == DT_DIR)
			{
				directorys.emplace_back(path + "/" + ptr->d_name);
			}
			else if (ptr->d_type == DT_REG)
			{
				files.emplace_back(path + "/" + ptr->d_name);
			}
		}
		return true;
#endif
	}

	bool dir::GetFilePaths(const std::string& path, std::vector<std::string>& paths, bool r)
	{
		if (path.empty())
		{
			return false;
		}
#ifdef __OS_WIN__
		WIN32_FIND_DATA findFileData;
		std::string temPath = fmt::format("{}/*.*", path);
		HANDLE fileHandle = FindFirstFile(temPath.c_str(), &findFileData);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			temPath = help::text::Utf8ToGB2312(path) + "/*.*";
			fileHandle = FindFirstFile(temPath.c_str(), &findFileData);
			if(fileHandle == INVALID_HANDLE_VALUE)
			{
				return false;
			}
		}


		while (true)
		{
			if (findFileData.cFileName[0] != '.')
			{
				std::string name(findFileData.cFileName);
				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					std::string newDir = fmt::format("{}/{}", path, name);
					GetFilePaths(newDir, paths);
				}
				else
				{
					paths.emplace_back(fmt::format("{}/{}", path, name));
				}
			}
			//如果是当前路径或者父目录的快捷方式，或者是普通目录，则寻找下一个目录或者文件
			if (!FindNextFile(fileHandle, &findFileData))
			{
				break;
			}
		}
		FindClose(fileHandle);
		return true;
#else
		DIR *dir = opendir(path.c_str());
		if (dir == NULL)
		{
			return false;
		}

		while (true)
		{
			struct dirent *ptr = readdir(dir);
			if (ptr == NULL)
			{
				break;
			}
			if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			{
				continue;
			}
			else if (ptr->d_type == DT_DIR && r)
			{
				std::string strFile = path + "/" + ptr->d_name;
				GetFilePaths(strFile, paths);
			}
			else if (ptr->d_type == 8)///file
			{
				std::string strFile = path + "/" + ptr->d_name;
				paths.push_back(strFile);
			}
		}
		return closedir(dir) != -1;
#endif
	}

	bool dir::GetFilePaths(const std::string& dir, const std::string& format, std::vector<std::string>& paths)
	{
		std::vector<std::string> allPaths;
		if (!GetFilePaths(dir, allPaths))
		{
			return false;
		}
		std::string fileSuffix(format);
		for(const std::string & path : allPaths)
		{
			if(path.find_last_of(format) != std::string::npos)
			{
				std::string str = path.substr(path.size() - format.size());
				if(help::Str::Tolower(str) == help::Str::Tolower(fileSuffix))
				{
					paths.emplace_back(path);
				}
			}
		}
		return true;
	}

	bool dir::GetDirByPath(const std::string& str, std::string& director)
	{
		size_t pos = str.find_last_of("/\\");
		if (pos == std::string::npos)
		{
			return false;
		}
		director = str.substr(0, pos + 1);
		return true;
	}

	bool dir::GetFileName(const std::string& fullName, std::string& fileName)
	{
		size_t pos = fullName.find_last_of("/\\");
		if (pos == std::string::npos)
		{
			return false;
		}
		fileName = fullName.substr(pos + 1);
		return true;
	}

	bool dir::GetDirAndFileName(const std::string& path, std::string& director, std::string& fileName)
	{
		size_t pos = path.find_last_of("/\\");
		if (pos == std::string::npos)
		{
			return false;
		}
		director = path.substr(0, pos + 1);
		fileName = path.substr(pos + 1);
		return true;
	}
}
