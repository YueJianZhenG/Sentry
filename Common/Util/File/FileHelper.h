#pragma once

#include <string>
#include <vector>
#include <fstream>
namespace help
{
    namespace fs
    {
		extern bool FileIsExist(const std::string &path);
		extern long long GetLastWriteTime(const std::string & path);
		extern bool GetFileLine(const std::string & path, size_t & size);
		extern bool GetFileSize(const std::string & path, size_t & size);
		extern bool GetFileSize(const std::string & path, std::string & size);
		extern bool GetFileName(const std::string& path, std::string& name);
		extern bool GetFileType(const std::string & path, std::string & type);
        extern bool ReadTxtFile(const std::string &path, std::string &outFile);
		extern bool ChangeName(const std::string & path, const std::string & name);
		extern bool WriterFile(const std::string &path, const std::string &fileContent);
		extern bool ReadTxtFile(const std::string &path, std::vector<std::string> &outLines, char delim = ';');


		extern bool Open(std::ifstream & fs, const std::string & path, std::ios_base::openmode flag = std::ios::in);
		extern bool Open(std::ofstream & fs, const std::string & path, std::ios_base::openmode flag = std::ios::out);
	}

}// namespace FileHelper
