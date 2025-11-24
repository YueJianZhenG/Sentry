//
// Created by 64658 on 2025/11/1.
//

#ifndef APP_COM_H
#define APP_COM_H
#include <string>

namespace help
{
	namespace com
	{
		constexpr size_t KB = 1024;
		constexpr size_t MB = 1024 * 1024;
		constexpr size_t GB = 1024 * 1024 * 1024;
		std::string BytesToString(long long size);
	}
}

#endif //APP_COM_H
