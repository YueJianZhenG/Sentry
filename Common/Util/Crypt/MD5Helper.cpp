//
// Created by 64658 on 2025/1/17.
//

#include <sstream>
#include <iomanip>
#include "MD5Helper.h"
#include "Util/Src/md5.h"
namespace help
{
	std::string md5::GetMd5(const std::string& input)
	{
		return GetMd5(input.c_str(), input.size());
	}

	std::string md5::GetMd5(const char* input, size_t size)
	{
		char buffer[HASHSIZE] = { 0 };
		::md5(input, (int)size, buffer);
		return std::string(buffer, sizeof(buffer));
	}

	std::string md5::GetHex(const std::string& input)
	{
		return md5::GetHex(input.c_str(), input.size());
	}

	std::string md5::GetHex(const char* input, size_t size)
	{
		std::ostringstream oss;
		unsigned char digest[HASHSIZE] = { 0 };
		::md5(input, (long)size, (char *)digest);
		for (int i = 0; i < HASHSIZE; ++i) {
			oss << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)digest[i];
		}
		return oss.str();
	}

	bool md5::FileMd5(const std::string& path, std::string& md5)
	{
		return ::file_md5(path, md5);
	}
}