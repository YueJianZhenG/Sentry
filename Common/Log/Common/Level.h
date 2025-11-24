//
// Created by yy on 2023/8/12.
//

#ifndef APP_LEVEL_H
#define APP_LEVEL_H
#include<string>
#include<memory>
#ifdef __SHARE_PTR_COUNTER__
#include "Core/Memory/MemoryObject.h"
#endif
namespace custom
{
	enum class LogLevel : unsigned char
	{
		None = LOG_LEVEL_NONE,
		Debug = LOG_LEVEL_DEBUG,
		Info = LOG_LEVEL_INFO,
		Warn = LOG_LEVEL_WARN,
		Error = LOG_LEVEL_ERROR,
		Fatal = LOG_LEVEL_FATAL,
		All,
	};

	struct LogInfo
#ifdef __SHARE_PTR_COUNTER__
	: public memory::Object<LogInfo>
#endif
	{
	public:
		LogLevel Level;
		bool Flush = false;
		std::string File;
		std::string Content;
		std::unique_ptr<std::string> Stack;
	public:
		inline void Clear()
		{
			this->File.clear();
			this->Stack.reset();
			this->Content.clear();
			this->Level = LogLevel::None;
		}
	};
}

#endif //APP_LEVEL_H
