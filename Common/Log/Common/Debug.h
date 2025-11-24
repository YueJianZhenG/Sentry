#pragma once

#include<string>
#include"Level.h"
#include"fmt.h"

namespace Debug
{
#ifdef __OS_WIN__
	extern bool Init();
	extern void Clear();
#endif
	extern void LuaError(const char * str);
	extern int Backtrace(std::string & trace, void * thread = nullptr);
	extern void Console(const custom::LogInfo & log);
	extern void Console(custom::LogLevel level, int code);
	extern void Log(std::unique_ptr<custom::LogInfo>& log);
	extern void Console(const std::string & name, const custom::LogInfo & log);
	extern void Log(const std::string & name, std::unique_ptr<custom::LogInfo>& log);
	extern void Print(custom::LogLevel level, const std::string & log);
}