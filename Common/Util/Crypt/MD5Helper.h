//
// Created by 64658 on 2025/1/17.
//

#ifndef APP_MD5HELPER_H
#define APP_MD5HELPER_H

#include<string>
namespace help
{
	namespace md5
	{
		extern std::string GetMd5(const std::string & input);
		extern std::string GetHex(const std::string & input);
		extern std::string GetMd5(const char * input, size_t size);
		extern std::string GetHex(const char * input, size_t size);
		extern bool FileMd5(const std::string & path, std::string & md5);
	}
}

#endif //APP_MD5HELPER_H
