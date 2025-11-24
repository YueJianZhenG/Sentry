#pragma once
#include<string>
#include<cstring>
#include"Log/Common/Debug.h"

inline std::string FormatFileLine(const char * file, const int line)
{
	const char* filename = strrchr(file, '/');
	if(filename == nullptr)
	{
		filename = strrchr(file, '\\');
	}

	if(filename != nullptr)
	{
		return fmt::format("{}:{}", filename + 1, line);
	}
	return fmt::format("{}:{}", file, line);
}



#if LOG_LEVEL_INFO >= SET_LOG_LEVEL
#define LOG_INFO(...) \
{                             \
		std::unique_ptr<custom::LogInfo> log = std::make_unique<custom::LogInfo>();          \
		log->Content = fmt::format(__VA_ARGS__);                \
    	log->Level = custom::LogLevel::Info;                   \
		log->File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Log(log);    									\
}
#else
#define LOG_INFO(...)
#endif


#if LOG_LEVEL_DEBUG >= SET_LOG_LEVEL
#define LOG_DEBUG(...) \
{                             \
		std::unique_ptr<custom::LogInfo> log = std::make_unique<custom::LogInfo>();          \
		log->Content = fmt::format(__VA_ARGS__);                \
    	log->Level = custom::LogLevel::Debug;                   \
		log->File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Log(log);    									\
}
#else
#define LOG_DEBUG(...)
#endif

#if LOG_LEVEL_WARN >= SET_LOG_LEVEL
#define LOG_WARN(...) \
{                             \
		std::unique_ptr<custom::LogInfo> log = std::make_unique<custom::LogInfo>();          \
		log->Content = fmt::format(__VA_ARGS__);                \
    	log->Level = custom::LogLevel::Warn;                   \
		log->File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Log(log);    									\
}
#else
#define LOG_WARN(...)
#endif

#if LOG_LEVEL_ERROR >= SET_LOG_LEVEL
#define LOG_ERROR(...) \
{                                      \
 		std::unique_ptr<custom::LogInfo> log = std::make_unique<custom::LogInfo>();          \
		log->Content = fmt::format(__VA_ARGS__);                \
    	log->Level = custom::LogLevel::Error;                   \
		log->File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Log(log);    									\
}
#else
#define LOG_ERROR(...)
#endif

#if LOG_LEVEL_ERROR >= SET_LOG_LEVEL
#define LOG_FATAL(...) \
{                                      \
       	std::unique_ptr<custom::LogInfo> log = std::make_unique<custom::LogInfo>();          \
		log->Content = fmt::format(__VA_ARGS__);                \
    	log->Level = custom::LogLevel::Fatal;                   \
		log->File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Log(log);    									\
}
#else
#define LOG_FATAL(...)
#endif


#define LOG_BY_NAME(name, ...) \
{                                      \
	std::unique_ptr<custom::LogInfo> log = std::make_unique<custom::LogInfo>();          \
	log->Content = fmt::format(__VA_ARGS__);                \
	log->Level = custom::LogLevel::None;                   \
	log->File = FormatFileLine(__FILE__, __LINE__);   		\
	Debug::Log(name, log);    									\
}

#define CONSOLE_BY_NAME(name, ...) \
{                                      \
	std::unique_ptr<custom::LogInfo> log = std::make_unique<custom::LogInfo>();          \
	log->Content = fmt::format(__VA_ARGS__);                \
	log->Level = custom::LogLevel::None;                   \
	log->File = FormatFileLine(__FILE__, __LINE__);   		\
	Debug::Console(name, log);    									\
}



#define LOG_CHECK_RET(obj) \
{                                \
    if(!(obj)) { LOG_ERROR(#obj); return; }    \
}

#define LOG_CHECK_RET_FALSE(obj) \
{                                \
    if(!(obj)) { LOG_ERROR(#obj); return false; }    \
}

#define CHECK_FALSE_RETURN(obj, value) \
{                                \
    if(!(obj)) { return value; }    \
}

#define LOG_CHECK_RET_NULL(obj){ \
    if(!(obj)) { LOG_ERROR(#obj); return nullptr; }    \
}

#define LOG_ERROR_RETURN_CODE(obj, code) \
    if(!(obj)) { LOG_ERROR(#obj); return code; }    \

#define LOG_ERROR_CHECK_ARGS(obj) \
    if(!(obj)) { LOG_ERROR(#obj); return XCode::CallArgsError; } \

#define CHECK_ARGS(obj) \
    if(!(obj)) { return XCode::CallArgsError; } \

#define CONSOLE_LOG_ERROR(...){ \
		custom::LogInfo log;          \
		log.Content = fmt::format(__VA_ARGS__);                \
    	log.Level = custom::LogLevel::Error;                   \
		log.File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Console(log);    									\
}

#define CONSOLE_LOG_FATAL(...){ \
		custom::LogInfo log;          \
		log.Content = fmt::format(__VA_ARGS__);                \
    	log.Level = custom::LogLevel::Fatal;                   \
		log.File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Console(log);    									\
}

#define CONSOLE_LOG_INFO(...){ \
		custom::LogInfo log;          \
		log.Content = fmt::format(__VA_ARGS__);                \
    	log.Level = custom::LogLevel::Info;                   \
		log.File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Console(log);    									\
}

#define CONSOLE_LOG_DEBUG(...) \
{ \
		custom::LogInfo log;          \
		log.Content = fmt::format(__VA_ARGS__);                \
    	log.Level = custom::LogLevel::Debug;                   \
		log.File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Console(log);    									\
}

#define CONSOLE_LOG_WARN(...) \
{ \
		custom::LogInfo log;          \
		log.Content = fmt::format(__VA_ARGS__);                \
    	log.Level = custom::LogLevel::Warn;                   \
		log.File = FormatFileLine(__FILE__, __LINE__);   		\
		Debug::Console(log);    									\
}

#define CONSOLE_ERROR_RETURN_CODE(obj, code) \
    if(!(obj)) { CONSOLE_LOG_ERROR(#obj); return code; }    \

#define CONSOLE_ERROR_RETURN_FALSE(obj) \
    if(!(obj)) { CONSOLE_LOG_ERROR(#obj); return false; }    \

#define IF_NOT_NULL_CALL(module, func, ...) { if(module != nullptr) { module->func(__VA_ARGS__); } };