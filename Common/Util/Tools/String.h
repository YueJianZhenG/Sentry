
#pragma once

#include <string>
#include <vector>
namespace help
{
    namespace Str
    {
        extern std::string EmptyStr;

        extern const std::string & Empty();

		extern bool IsPhoneNumber(const std::string & phone);

		extern size_t Hash(const std::string & str);

		extern bool IsRegex(const std::string & pattern);

        //转大写
        extern const std::string & Toupper(std::string & str);

        //转小写
        extern const std::string & Tolower(std::string & str);

        extern void ClearBlank(std::string &input);

        extern std::string RandomString(int size = 64);

        extern bool GetFileName(const std::string &path, std::string & name);

        extern std::string FormatJson(const std::string &json);

        extern void Replace(std::string &outstring, const std::string& str1, const std::string& str2);

		extern size_t Split(const std::string &target, char cc, std::string & str1, std::string & str2);

    	extern size_t Split(const std::string &targetString, char cc, std::vector<int> &ret);

		extern size_t Split(const std::string &targetString, char cc, std::vector<std::string> &ret);

    	template<typename ... Args>
    	inline bool Scanf(const char * str, const std::string & fmt, Args &&... args)
    	{
    		int count = 0;
#ifndef _MSC_VER
    		count = sscanf(str, fmt.c_str(), std::forward<Args>(args)...);
#else
    		count = sscanf_s(str, fmt.c_str(), std::forward<Args>(args)...);
#endif
    		return count == sizeof ...(Args);
    	}
    }

    namespace Str
    {
		extern bool IsIpAddress(const std::string & str);
		extern bool IsRpcAddr(const std::string & address);
		extern bool IsHttpAddr(const std::string & address);
		extern bool SplitAddr(const std::string& address, std::string& ip, unsigned short& port);
		extern bool SplitAddr(const std::string& address, std::string & net, std::string& ip, unsigned short& port);
    }
}// namespace StringHelper


namespace help
{
	namespace utf8
	{
		extern bool IsByte(unsigned char cc);
		extern bool IsValid(const std::string & str);
		extern size_t Length(const std::string & str);
		extern std::string Sub(const std::string & str, size_t n);
		extern std::string Sub(const std::string & str, size_t s, size_t n);
	}
#ifdef __OS_WIN__
	namespace text
	{
		extern std::string GB2312ToUtf8(const std::string & text);
		extern std::string Utf8ToGB2312(const std::string & text);
	}
#endif
}