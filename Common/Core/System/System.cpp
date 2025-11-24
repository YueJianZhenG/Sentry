//
// Created by zmhy0073 on 2022/10/13.
//

#include "System.h"
#include <regex>
#ifdef __OS_WIN__

#include<direct.h>
#include<Windows.h>
#include <psapi.h>
#include<iostream>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "kernel32.lib")
#endif

#ifdef __OS_MAC__
#include<unistd.h>
#include<mach/mach.h>
#include<sys/sysctl.h>
#endif

#ifdef __OS_LINUX__
#include<unistd.h>
#include<fstream>
#include<cstdint>
#include<sys/sysinfo.h>
#include<sys/resource.h>
//#include<memory_resource>
#endif

#include<cstdlib>
#include<fstream>
#include"fmt.h"
#include"Util/Tools/String.h"
#include"Log/Common/CommonLogDef.h"

#ifdef __OS_WIN__

#include <dbghelp.h>
#include <tchar.h>
#include <Util/Tools/TimeHelper.h>

#pragma comment(lib, "dbghelp.lib")

namespace os
{
	bool CreateMiniDump(EXCEPTION_POINTERS* pep)
	{
		// 打开文件
		std::string name = fmt::format("{}.dmp", help::Time::GetDateStr());
		HANDLE hFile = CreateFile(_T(name.c_str()), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		// 创建转储文件
		MINIDUMP_EXCEPTION_INFORMATION mei;
		mei.ThreadId = GetCurrentThreadId();
		mei.ExceptionPointers = pep;
		mei.ClientPointers = FALSE;

		BOOL result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL,
				NULL);
		CloseHandle(hFile);
		return result != 0;
	}

	LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionPtrs)
	{
		CreateMiniDump(pExceptionPtrs);
		return EXCEPTION_EXECUTE_HANDLER;
	}
}
#endif

namespace os
{
	bool System::Init(int argc, char** argv)
	{
		std::vector<std::string> commandLine;
		commandLine.reserve(argc);
		for (int index = 0; index < argc; index++)
		{
			commandLine.emplace_back(argv[index]);
		}
		System::SetAppEnv("id", "1"); //服务器id
		System::SetAppEnv("node", "all");
		System::SetAppEnv("name", "server");
		System::SetAppEnv("db", "127.0.0.1"); //db地址
		System::SetAppEnv("config", "./config/run/all.json");

		char buffer[256] = { 0 };
#ifdef __OS_WIN__
		std::string work = fmt::format("{0}/", _getcwd(buffer, sizeof(buffer)));
#else
		std::string work = fmt::format("{0}/", getcwd(buffer, sizeof(buffer)));
#endif
		help::Str::Replace(work, "\\", "/");
		if (work.back() == '/')
		{
			work.pop_back();
		}
		System::mWorkPath = work;
		std::vector<std::string> result;
		for (size_t index = 1; index < commandLine.size(); index++)
		{
			result.clear();
			std::string line(commandLine[index]);
			if (line.find("--") == std::string::npos)
			{
				CONSOLE_LOG_ERROR("args:{} format error", line);
				return false;
			}
			const std::string str = line = line.substr(2);
			if (help::Str::Split(str, '=', result) != 2)
			{
				CONSOLE_LOG_ERROR("args:{} format error", line);
				return false;
			}
			System::SetAppEnv(result[0], result[1]);
		}
		std::string path;
		System::GetAppEnv("CONFIG", path);
		CONSOLE_LOG_DEBUG("config path = {}", path);
#ifdef __OS_WIN__
		SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
#endif
		return System::AddValue("WORK_DIR", work);
	}

	bool System::AddValue(const std::string& key, const std::string& value)
	{
		char buffer[256] = { 0 };
		sprintf(buffer, "${%s}", key.c_str());
		mSubValues[std::string(buffer)] = value;
		return true;
	}

	void System::LuaSetEnv(const char* key, const char* val)
	{
		System::SetEnv(key, val);
	}

	void System::LuaSetAppEnv(const char* key, const char* val)
	{
		System::SetAppEnv(key, val);
	}

	bool System::GetAppEnv(const std::string& k, int& v)
	{
		std::string value;
		if (!System::GetAppEnv(k, value))
		{
			return false;
		}
		v = std::stoi(value);
		return true;
	}

	bool System::GetAppEnv(const std::string& k, std::string& v)
	{
		std::string str = fmt::format("APP_{}", k);
		help::Str::Toupper(str);
		const char* val = getenv(str.c_str());
		if (val == nullptr)
		{
			return false;
		}
		v = val;
		return true;
	}

	bool System::SubValue(std::string& value)
	{
		std::smatch match;
		std::regex re(R"(\$?\{[^}]*\})");   // 匹配 ${...} 或 $${...} 格式的字符串
		if (std::regex_search(value, match, re))
		{
			auto iter = mSubValues.begin();
			for (; iter != mSubValues.end(); iter++)
			{
				const std::string& key = iter->first;
				const std::string& val = iter->second;
				help::Str::Replace(value, key, val);
			}
			return true;
		}
		return false;
	}

	bool System::Run(const std::string& cmd, std::list<std::string> & output)
	{
#ifdef __OS_WIN__
		FILE* pipe = _popen(cmd.c_str(), "r");
#else
		FILE* pipe = popen(cmd.c_str(), "r");
#endif
		if (!pipe)
		{
			return false;
		}
		size_t count = 0;
		char buffer[1024] = { 0 };
		while (!feof(pipe) && count <= 1000)
		{
			count++;
			size_t len = fread(buffer, 1, sizeof(buffer), pipe);
			if (len > 0)
			{
				output.emplace_back(buffer, len);
			}
		}
#ifdef __OS_WIN__
		_pclose(pipe);  // 关闭管道
#else
		pclose(pipe);  // 关闭管道
#endif
		return true;

	}

	void System::SetAppEnv(const std::string& k, const std::string& v)
	{
		std::string str = fmt::format("APP_{}", k);
		help::Str::Toupper(str);
#if WIN32
		_putenv_s(str.c_str(), v.c_str());
#else
		setenv(str.c_str(), v.c_str(), 1);
#endif
		System::AddValue(k, v);
	}

	void System::SetEnv(const std::string& k,const std::string& v)
	{
#if WIN32
		_putenv_s(k.c_str(), v.c_str());
#else
		setenv(k.c_str(), v.c_str(), 1);
#endif
	}

	bool System::ReadFile(const std::string& path, std::string& content)
	{
		std::ifstream fs;
		fs.open(path, std::ios::binary | std::ios::in);
		if (!fs.is_open())
		{
			return false;
		}
		std::string lineData;
		while (std::getline(fs, lineData))
		{
			System::SubValue(lineData);
			content.append(lineData);
			content += "\n";
		}
		return true;
	}

	std::string System::FormatPath(const std::string& path)
	{
		if (path[0] == '/')
		{
			return System::mWorkPath + path;
		}
		return fmt::format("{}/{}", mWorkPath, path);
	}

#ifdef __OS_LINUX__
	CPUUsage System::GetCPUUsage()
	{
		 CPUUsage usage = {0};
    	// 获取系统总CPU时间
    	std::ifstream stat("/proc/stat");
    	std::string line;
    	std::getline(stat, line);
    	std::istringstream iss_sys(line);
    	std::string label;
    	iss_sys >> label;
    	if (label == "cpu") {
        	unsigned long val;
        	while (iss_sys >> val) usage.total += val;
    	}

    	// 获取进程CPU时间
    	std::ifstream proc("/proc/self/stat");
    	std::getline(proc, line);
    	size_t rparen = line.rfind(')');
    	std::istringstream iss_proc(line.substr(rparen+2));

    	std::vector<std::string> fields;
    	std::string field;
		while (iss_proc >> field) fields.push_back(field);

    	if (fields.size() > 13) {
        	usage.utime = stoul(fields[11]); // utime在字段14
        	usage.stime = stoul(fields[12]); // stime在字段15
    	}
    	return usage;
	}
#endif

	bool System::GetSystemInfo(SystemInfo& systemInfo)
	{
// 初始化默认值
		systemInfo.cpu = 0.0;
		systemInfo.use_memory = 0;
		systemInfo.max_memory = 0;

#ifdef __OS_MAC__
		// 获取当前进程内存使用量
	mach_task_basic_info_data_t taskInfo;
	mach_msg_type_number_t taskInfoCount = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&taskInfo, &taskInfoCount) != KERN_SUCCESS) {
		return false;
	}
	systemInfo.use_memory = taskInfo.resident_size;

	// 获取系统总内存
	int mib[2] = {CTL_HW, HW_MEMSIZE};
	uint64_t physicalMemorySize = 0;
	size_t len = sizeof(physicalMemorySize);
	if (sysctl(mib, 2, &physicalMemorySize, &len, nullptr, 0) != 0) {
		return false;
	}
	systemInfo.max_memory = physicalMemorySize;

	// 获取 CPU 使用率（考虑所有线程）
	thread_array_t threadList;
	mach_msg_type_number_t threadCount;
	if (task_threads(mach_task_self(), &threadList, &threadCount) != KERN_SUCCESS) {
		return false;
	}

	double totalCpuUsage = 0.0;
	for (mach_msg_type_number_t i = 0; i < threadCount; ++i) {
		thread_basic_info_data_t threadInfo;
		mach_msg_type_number_t threadInfoCount = THREAD_BASIC_INFO_COUNT;
		if (thread_info(threadList[i], THREAD_BASIC_INFO, (thread_info_t)&threadInfo, &threadInfoCount) == KERN_SUCCESS) {
			totalCpuUsage += static_cast<double>(threadInfo.cpu_usage);
		}
	}
	// 释放线程列表
	vm_deallocate(mach_task_self(), (vm_address_t)threadList, threadCount * sizeof(thread_t));

	// 获取 CPU 核心数
	host_basic_info_data_t hostInfo;
	mach_msg_type_number_t hostInfoCount = HOST_BASIC_INFO_COUNT;
	if (host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostInfo, &hostInfoCount) != KERN_SUCCESS) {
		return false;
	}
	unsigned int processorCount = hostInfo.max_cpus;

	// 计算 CPU 使用率 (THREAD_USAGE_SCALE = 1000)
	systemInfo.cpu = (totalCpuUsage / (1000.0 * processorCount)) * 100.0;

#elif __OS_LINUX__

		std::ifstream status("/proc/self/status");
		std::string line;
		while (std::getline(status, line)) {
			if (line.compare(0, 6, "VmRSS:") == 0) {
				size_t kb;
				std::istringstream iss(line.substr(6));
				iss >> kb;
				systemInfo.use_memory = kb * 1024; // 转换为字节
			}
		}

		std::ifstream meminfo("/proc/meminfo");
		while (std::getline(meminfo, line)) {
			if (line.compare(0, 9, "MemTotal:") == 0) {
				size_t kb;
				std::istringstream iss(line.substr(9));
				iss >> kb;
				systemInfo.max_memory = kb * 1024; // 转换为字节
			}
		}

		static CPUUsage lateInfo = System::GetCPUUsage();

		CPUUsage currInfo = System::GetCPUUsage();

		 const unsigned long process_diff =
        (currInfo.utime + currInfo.stime) - (lateInfo.utime + lateInfo.stime);
    	const unsigned long total_diff = currInfo.total - lateInfo.total;

    	if (total_diff > 0) {
			systemInfo.cpu = (process_diff * 100.0f) / total_diff;
		}
		lateInfo = currInfo;
#else // Windows
		// 静态变量用于记录上一次时间
		static ULONGLONG lastSysTime = 0;
		static ULONGLONG lastProcTime = 0;

		HANDLE hProcess = GetCurrentProcess();
		if (hProcess == nullptr)
		{
			return false;
		}

		FILETIME ftSysIdle, ftSysKernel, ftSysUser, ftProcCreation, ftProcExit, ftProcKernel, ftProcUser;
		bool success = false;

		// 获取 CPU 使用率
		if (GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser) &&
			GetProcessTimes(hProcess, &ftProcCreation, &ftProcExit, &ftProcKernel, &ftProcUser))
		{
			ULONGLONG sysKernel = ((ULONGLONG)ftSysKernel.dwHighDateTime << 32) | ftSysKernel.dwLowDateTime;
			ULONGLONG sysUser = ((ULONGLONG)ftSysUser.dwHighDateTime << 32) | ftSysUser.dwLowDateTime;
			ULONGLONG procKernel = ((ULONGLONG)ftProcKernel.dwHighDateTime << 32) | ftProcKernel.dwLowDateTime;
			ULONGLONG procUser = ((ULONGLONG)ftProcUser.dwHighDateTime << 32) | ftProcUser.dwLowDateTime;

			ULONGLONG sysTime = sysKernel + sysUser;
			ULONGLONG procTime = procKernel + procUser;

			if (lastSysTime > 0 && sysTime > lastSysTime)
			{
				ULONGLONG sysTimeDiff = sysTime - lastSysTime;
				ULONGLONG procTimeDiff = procTime - lastProcTime;
				systemInfo.cpu = (procTimeDiff * 100.0) / sysTimeDiff;
			}
			lastSysTime = sysTime;
			lastProcTime = procTime;
			success = true;
		}

		// 获取内存使用量
		PROCESS_MEMORY_COUNTERS pmc = { 0 };
		pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			systemInfo.use_memory = pmc.WorkingSetSize;
			success = true;
		}

		// 获取系统总内存
		MEMORYSTATUSEX memStatus = { 0 };
		memStatus.dwLength = sizeof(memStatus);
		if (GlobalMemoryStatusEx(&memStatus))
		{
			systemInfo.max_memory = memStatus.ullTotalPhys;
			success = true;
		}

		CloseHandle(hProcess);
		return success;

#endif
		return true;
	}

	std::string System::mWorkPath;
	std::unordered_map<std::string, std::string> System::mSubValues;
}