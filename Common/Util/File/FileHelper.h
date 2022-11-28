#pragma once
#include "rapidjson/document.h"
#include <string>
#include <vector>

namespace Helper
{
    namespace File {
        extern bool FileIsExist(const std::string &path);

        extern bool ReadTxtFile(const std::string &path, std::string &outFile);

        extern bool ReadTxtFile(const std::string &path, std::string &outFile, std::string & md5);

        extern bool ReadJsonFile(const std::string &path, rapidjson::Document &document);

        extern bool ReadJsonFile(const std::string &path, rapidjson::Document &document, std::string &md5);

        extern bool ReadTxtFile(const std::string &path, std::vector<std::string> &outLines);

        extern bool WriterFile(const std::string &path, const std::string &fileContent);
    }

}// namespace FileHelper
