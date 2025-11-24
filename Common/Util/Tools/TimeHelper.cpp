#include"TimeHelper.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "String.h"
using namespace std::chrono;
constexpr const char* TIME_FORMAT = "%Y-%m-%d %H:%M:%S";
constexpr const char* TIME_FORMAT_NUM = "%Y%m%d%H%M%S";

namespace help
{
	long long Time::TimeOffset = 0;

	long long Time::NowTimeMS = 0;

	inline std::tm GetUtcTM(time_t now)
	{
		std::tm tm{};
#ifdef __OS_WIN__
		gmtime_s(&tm, &now);
#else
		gmtime_r(&now, &tm);
#endif
		return tm;
	}

	inline std::tm GetLocalTM(time_t now)
	{
		std::tm tm{};
#ifdef __OS_WIN__
		localtime_s(&tm, &now);
#else
		localtime_r(&now, &tm);
#endif
		return tm;
	}

	Time::Date Time::GetTimeDate(long long t)
	{
		Time::Date timeDate;
		time_t t1 = t == 0 ? NowSec() : t;
		std::tm tm1 = help::GetLocalTM(t1);
		{
			timeDate.day = tm1.tm_mday;
			timeDate.hour = tm1.tm_hour;
			timeDate.minute = tm1.tm_min;
			timeDate.second = tm1.tm_sec;
			timeDate.days = tm1.tm_yday;
			timeDate.month = tm1.tm_mon + 1;
			timeDate.week = tm1.tm_wday + 1;
			timeDate.year = tm1.tm_year + 1900;
		}
		return timeDate;
	}

	bool Time::IsSameDay(long long t1, long long t2)
	{
		std::tm tm1 = help::GetLocalTM(t1);
		std::tm tm2 = help::GetLocalTM(t2);
		return tm1.tm_yday == tm2.tm_yday;
	}

	void Time::SetOffsetTime(int second)
	{
		if (second == 0)
		{
			Time::TimeOffset = 0;
			return;
		}
		Time::TimeOffset += second;
	}

	void Time::SetOffsetTime(const char * time_str)
	{
		if (time_str == nullptr)
		{
			Time::TimeOffset = 0;
			return;
		}
		time_t targetTime = 0;
		int hour = 0, minute = 0, second = 0;
		if (help::Str::Scanf(time_str, "%d:%d:%d", &hour, &minute, &second))
		{
			targetTime = help::Time::GetNewTime(hour, minute, second);
		}
		else
		{
			targetTime = help::Time::GetTimeByString(time_str);
		}
		if (targetTime <= 0)
		{
			return;
		}
		Time::TimeOffset = targetTime - Time::NowSystemSec();
	}

	std::string Time::GetDateStr(long long time)
	{
		char str[100] = {0};
		time_t t = (time_t)(time == 0
			                    ? Time::NowSec()
			                    : time);
		struct tm pt = GetLocalTM(t);
		size_t size = strftime(str, sizeof(str), TIME_FORMAT_NUM, &pt);
		return std::string(str, size);
	}

	long long Time::GetTimeByString(const char* timeStr)
	{
		std::tm tm = {};
		std::istringstream ss(timeStr);
		ss >> std::get_time(&tm, TIME_FORMAT);
		return ss.fail() ? 0 : std::mktime(&tm);
	}

	long long Time::NowMil()
	{
		auto tmp = system_clock::now().time_since_epoch();
		return duration_cast<milliseconds>(tmp).count() + Time::TimeOffset * 1000;
	}

	long long Time::NowSec()
	{
		auto tmp = system_clock::now().time_since_epoch();
		return duration_cast<seconds>(tmp).count() + Time::TimeOffset;
	}

	long long Time::NowSystemSec()
	{
		auto tmp = system_clock::now().time_since_epoch();
		return duration_cast<seconds>(tmp).count();
	}

	long long Time::GetNextDayTime()
	{
		std::tm t = GetLocalTM(NowSec());
		{
			t.tm_hour = 0;
			t.tm_min = 0;
			t.tm_sec = 0;
		}
		return mktime(&t) + Time::DaySecond;
	}

	long long Time::GetNextHourTime()
	{
		std::tm t = GetLocalTM(NowSec());
		{
			t.tm_hour += 1;
			t.tm_min = 0;
			t.tm_sec = 0;
		}
		return mktime(&t);
	}

	long long Time::GetNextMinuteTime()
	{
		std::tm t = GetLocalTM(NowSec());
		{
			t.tm_min += 1;
			t.tm_sec = 0;
		}
		return mktime(&t);
	}

	long long Time::GetNewTime(int hour, int minute, int second)
	{
		std::tm t = GetLocalTM(NowSec());
		{
			t.tm_hour = hour;
			t.tm_min = minute;
			t.tm_sec = second;
		}
		return mktime(&t);
	}

	void Time::GetHourMinSecond(long long sec, int& hour, int& min, int& second)
	{
		std::tm time = GetLocalTM(sec);
		{
			hour = time.tm_hour;
			min = time.tm_min;
			second = time.tm_sec;
		}
	}

	void Time::GetHourMinSecond(long long sec, int time[3])
	{
		std::tm tm = GetLocalTM(sec);
		time[0] = tm.tm_hour;
		time[1] = tm.tm_min;
		time[2] = tm.tm_sec;
	}

	std::string Time::GetDateString(long long time)
	{
		char str[100] = {0};
		time_t t = time == 0 ? NowSec() : time;
		struct std::tm pt = GetLocalTM(t);
		size_t size = std::strftime(str, sizeof(str), TIME_FORMAT, &pt);
		return {str, size};
	}

	std::string Time::GetGMTDateString(long long time)
	{
		char str[100] = {0};
		time_t t = time == 0 ? NowSec() : time;
		struct tm pt = help::GetUtcTM(t);
		size_t size = strftime(str, sizeof(str), "%Y-%m-%dT%H:%M:%SZ", &pt);
		return {str, size};
	}

	std::string Time::GetDateGMT(long long time)
	{
		time_t t = time == 0 ? NowSec() : time;

		std::stringstream ss;
		struct tm gmt = GetLocalTM(t);
		ss << std::put_time(&gmt, "%a, %d %b %Y %H:%M:%S GMT");
		return ss.str();
	}

	std::string Time::GetDateDT(long long timestamp)
	{
		if (timestamp == 0)
		{
			timestamp = NowSec();
		}
		char buffer[100] = {0}; // 缓冲区大小
		std::tm tm = GetLocalTM(timestamp); // 转换为 UTC 时间
		size_t size = std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S.000+08:00", &tm); // 格式化为 ISO8601 格式
		return std::string(buffer, size);
	}

	std::string Time::GetDateISO(long long timestamp)
	{
		if (timestamp == 0)
		{
			timestamp = NowSec();
		}
		char buffer[100] = {0}; // 缓冲区大小
		std::tm tm = GetLocalTM(timestamp); // 转换为 UTC 时间
		size_t size = std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S.000Z", &tm); // 格式化为 ISO8601 格式
		return std::string(buffer, size);
	}

	std::string Time::GetYearMonthDayString(long long t)
	{
		if (t == 0)
		{
			t = NowSec();
		}
		time_t t1 = t;
		char str[100] = {0};
		struct tm pt = GetLocalTM(t1);
		size_t size = strftime(str, sizeof(str), "%Y-%m-%d", &pt);
		return {str, size};
	}

	Time::Date Time::CalcHourMinSecond(long long totalMilliseconds)
	{
		Time::Date timeData;
		constexpr int secondsInMinute = 60;
		constexpr int secondsInHour = 60 * secondsInMinute;
		constexpr int secondsInDay = 24 * secondsInHour;
		constexpr int daysInMonth = 30;
		constexpr int daysInYear = 365;

		// 计算年数
		timeData.year = totalMilliseconds / (secondsInDay * daysInYear);
		totalMilliseconds %= (secondsInDay * daysInYear);

		// 计算月数
		timeData.month = totalMilliseconds / (secondsInDay * daysInMonth);
		totalMilliseconds %= (secondsInDay * daysInMonth);

		// 计算天数
		timeData.day = totalMilliseconds / secondsInDay;
		totalMilliseconds %= secondsInDay;

		// 计算小时数
		timeData.hour = totalMilliseconds / secondsInHour;
		totalMilliseconds %= secondsInHour;

		// 计算分钟数
		timeData.minute = totalMilliseconds / secondsInMinute;

		// 剩余的秒数
		timeData.second = totalMilliseconds % secondsInMinute;
		return timeData;
	}

	long long HighTime::NowSec()
	{
		auto tmp = high_resolution_clock::now().time_since_epoch();
		return duration_cast<std::chrono::seconds>(tmp).count() + Time::TimeOffset * 1000;
	}

	long long HighTime::NowMil()
	{
		auto tmp = high_resolution_clock::now().time_since_epoch();
		return duration_cast<milliseconds>(tmp).count() + Time::TimeOffset * 1000;
	}
}
