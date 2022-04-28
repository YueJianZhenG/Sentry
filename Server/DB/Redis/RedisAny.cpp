//
// Created by yjz on 2022/4/28.
//

#include "RedisAny.h"
namespace Sentry
{
	RedisLong::RedisLong(int value)
		: mValue(value)
	{

	}

	RedisLong::RedisLong(long long value)
		: mValue(value)
	{

	}
	void RedisLong::Write(std::iostream& io)
	{
		std::string str = std::to_string(this->mValue);
		io << '$' << str.size() << "\r\n" << str << "\r\n";
	}

	RedisString::RedisString(const std::string& value)
		: mValue(value)
	{

	}

	RedisString::RedisString(const char* str, size_t size)
		: mValue(str, size)
	{

	}

	void RedisString::Write(std::iostream& io)
	{
		io << '$' << this->mValue.size() << "\r\n" << this->mValue << "\r\n";
	}
}
