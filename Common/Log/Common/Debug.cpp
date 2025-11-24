//
// Created by zmhy0073 on 2022/9/22.
//
#include"Debug.h"
#include "Rang.h"
#ifdef __OS_LINUX__
#include<execinfo.h>
#endif

#ifndef __OS_WIN__
#include<cxxabi.h>
#else
#include <windows.h>
#include <dbghelp.h>
#endif

#include"Util/Tools/TimeHelper.h"
#include"Entity/Actor/App.h"
#include"Log/Component/LoggerComponent.h"
#include"XCode/XCode.h"
#include "Server/Config/CodeConfig.h"

using namespace acs;

#define DUMP_STACK_DEPTH_MAX 100

void Debug::LuaError(const char* str)
{
	//const char * log = strstr(str, ".lua");
	//Debug::Log(custom::LogLevel::Error, str);
}

#ifdef __OS_WIN__
#include "Util/Tools/String.h"

bool Debug::Init()
{
	const char* pbd = nullptr;
	// 启动符号，不然win上获取不到堆栈 必须有pdb文件
	return SymInitialize(GetCurrentProcess(), pbd, TRUE);
}

void Debug::Clear()
{
	SymCleanup(GetCurrentProcess());
}

#endif

void Debug::Log(std::unique_ptr<custom::LogInfo>& log)
{
	if (log->Level >= custom::LogLevel::Fatal)
	{
		log->Stack = std::make_unique<std::string>();
		Debug::Backtrace(*log->Stack);
	}

	static LoggerComponent* logComponent = nullptr;
	if (logComponent == nullptr)
	{
		logComponent = App::Get<LoggerComponent>();
		if (logComponent == nullptr)
		{
			Debug::Console(*log);
			return;
		}
	}
	logComponent->PushLog(log);
}

void Debug::Log(const std::string& name, std::unique_ptr<custom::LogInfo>& log)
{
	log->Level = custom::LogLevel::None;
	static LoggerComponent* logComponent = nullptr;
	if (logComponent == nullptr)
	{
		logComponent = App::Get<LoggerComponent>();
		if (logComponent == nullptr)
		{
			Debug::Console(name, *log);
			return;
		}
	}
	logComponent->PushLog(name, log);
}

#ifndef __OS_WIN__
void demangle(char* msg, std::string& out)
{
	char *mangled_name = nullptr, *offset_begin = nullptr, *offset_end = nullptr;

	for (char* p = msg; p && *p; ++p)
	{
		if (*p == '(')
		{
			mangled_name = p;
		}
		else if (*p == '+')
		{
			offset_begin = p;
		}
		else if (*p == ')')
		{
			offset_end = p;
			break;
		}
	}
	if (mangled_name && offset_begin && offset_end &&
		mangled_name < offset_begin)
	{
		*mangled_name++ = '\0';
		*offset_begin++ = '\0';
		*offset_end++ = '\0';

		int status;
		char* real_name = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);

		if (status == 0)
		{
			out = out + real_name + "+" + offset_begin + offset_end;
		}
		else
		{
			out = out + mangled_name + "+" + offset_begin + offset_end;
		}
		free(real_name);
	}
	else
		out += msg;
}

#endif

int Debug::Backtrace(std::string& trace, void* thread)
{
	int count = 0;
#ifdef __OS_LINUX__
	void* stack_trace[DUMP_STACK_DEPTH_MAX] = {0};
	char** stack_strings = nullptr;
	int stack_depth = 0;
	int i = 0;
	char index[5];

	stack_depth = backtrace(stack_trace, DUMP_STACK_DEPTH_MAX);
	stack_strings = (char**)backtrace_symbols(stack_trace, stack_depth);
	if (nullptr == stack_strings)
	{
		trace += " Memory is not enough while dump Stack Trace! \n";
		return 0;
	}
	for (i = 2; i < stack_depth; ++i)
	{
		count++;
		snprintf(index, sizeof(index), "#%02d ", i);
		trace += index;
		demangle(stack_strings[i], trace);
		trace += "\n";
	}

	free(stack_strings);
	stack_strings = nullptr;
#elif __OS_WIN__
	HANDLE process = GetCurrentProcess();
	if (thread == nullptr)
	{
		thread = GetCurrentThread();
	}
	CONTEXT context = {};
	context.ContextFlags = CONTEXT_FULL;
	RtlCaptureContext(&context);

	STACKFRAME64 stackFrame;
	memset(&stackFrame, 0, sizeof(STACKFRAME64));

#ifdef _M_IX86
	DWORD machineType = IMAGE_FILE_MACHINE_I386;
	stackFrame.AddrPC.Offset = context.Eip;
	stackFrame.AddrFrame.Offset = context.Ebp;
	stackFrame.AddrStack.Offset = context.Esp;
#elif _M_X64
	DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
	stackFrame.AddrPC.Offset = context.Rip;
	stackFrame.AddrFrame.Offset = context.Rbp;
	stackFrame.AddrStack.Offset = context.Rsp;
#endif

	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrStack.Mode = AddrModeFlat;

	for (int i = 0; i < 20; i++)
	{
		if (!StackWalk64(machineType, process, thread, &stackFrame, &context, nullptr, SymFunctionTableAccess64,
		                 SymGetModuleBase64, nullptr))
		{
			break;
		}

		if (stackFrame.AddrPC.Offset == 0)
		{
			break;
		}
		// 获取符号
		DWORD64 address = stackFrame.AddrPC.Offset;
		char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbolBuffer;
		symbol->MaxNameLen = MAX_SYM_NAME;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		DWORD64 displacement = 0;
		if (SymFromAddr(process, address, &displacement, symbol) && i >= 2)
		{
			IMAGEHLP_LINE64 line;
			DWORD displacementLine = 0;
			memset(&line, 0, sizeof(line));
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			if (SymGetLineFromAddr64(process, address, &displacementLine, &line))
			{
				count++;
				std::string file = FormatFileLine(line.FileName, line.LineNumber);
				trace.append(fmt::format("#{}: {} {:#x} {}\n", i - 2, symbol->Name, symbol->Address, file));
			}
		}
	}
#endif
	return count;
}

void Debug::Console(custom::LogLevel level, int code)
{
	if (code == XCode::Ok)
		return;
	custom::LogInfo logInfo;
	{
		logInfo.Level = level;
		const std::string& desc = acs::CodeConfig::Inst()->GetDesc(code);
		logInfo.Content = fmt::format("code = {}", desc);
	}
	Debug::Console(logInfo);
}

void Debug::Console(const custom::LogInfo& logInfo)
{
	constexpr char empty = ' ';
	const std::string& file = logInfo.File;
	std::string time = help::Time::GetDateString();
#ifndef __OS_WIN__
	const std::string& log = logInfo.Content;
#else
	std::string log = help::text::Utf8ToGB2312(logInfo.Content);
#endif
	switch (logInfo.Level)
	{
	case custom::LogLevel::Debug:
		{
			static std::string type = fmt::format(" [{:<7}] ", "debug");
			std::cout << rang::fg::green << time << type << file << empty << log << std::endl;
			break;
		}

	case custom::LogLevel::Info:
		{
			static std::string type = fmt::format(" [{:<7}] ", "info");
			std::cout << rang::fg::cyan << time << type << file << empty << log << std::endl;
			break;
		}


	case custom::LogLevel::Warn:
		{
			static std::string type = fmt::format(" [{:<7}] ", "warn");
			std::cout << rang::fg::yellow << time << type << file << empty << log << std::endl;
			break;
		}

	case custom::LogLevel::Error:
		{
			static std::string type = fmt::format(" [{:<7}] ", "error");
			std::cout << rang::fg::red << time << type << file << empty << log << std::endl;
			break;
		}

	case custom::LogLevel::Fatal:
		{
			static std::string type = fmt::format(" [{:<7}] ", "fatal");
			std::cout << rang::fg::magenta << time << type << file << empty << log << std::endl;
			break;
		}

	default:
		break;
	}
	if (logInfo.Stack != nullptr)
	{
		std::cout << rang::fgB::magenta << (*logInfo.Stack) << std::endl;
	}
}

void Debug::Console(const std::string& name, const custom::LogInfo& logInfo)
{
	constexpr char empty = ' ';
	const std::string& file = logInfo.File;
	std::string time = help::Time::GetDateString();
#ifndef __OS_WIN__
	const std::string& log = logInfo.Content;
#else
	std::string log = help::text::Utf8ToGB2312(logInfo.Content);
#endif
	std::string type = fmt::format(" [{:<7}] ", name);
	std::cout << rang::fg::gray << time << type << file << empty << log << std::endl;
}

void Debug::Print(custom::LogLevel level, const std::string& log)
{
	switch (level)
	{
	case custom::LogLevel::Info:
		std::cout << rang::fgB::cyan << log << std::endl;
		break;
	case custom::LogLevel::Debug:
		std::cout << rang::fgB::green << log << std::endl;
		break;
	case custom::LogLevel::Warn:
		std::cout << rang::fgB::yellow << log << std::endl;
		break;
	case custom::LogLevel::Error:
		std::cout << rang::fgB::red << log << std::endl;
		break;
	case custom::LogLevel::Fatal:
		std::cout << rang::fgB::magenta << log << std::endl;
		break;
	default:
		break;
	}
}
