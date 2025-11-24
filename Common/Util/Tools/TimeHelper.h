#pragma once
#include<ctime>
#include<string>


namespace help
{
    namespace Time
    {
		struct Date
		{
			int year = 0;
			int month = 0;
			int day = 0;
			int hour = 0;
			int minute = 0;
			int second = 0;
			int week = 0;
			int days = 0; // 0-356
		};


		constexpr int MinSecond = 60;
		constexpr int HourSecond = 60 * 60;
		constexpr int DaySecond = 60 * 60 * 24;

        extern long long TimeOffset; //时间偏移

    	extern long long NowTimeMS; //当前时间

		extern bool IsSameDay(long long t1, long long t2);

        extern void SetOffsetTime(int second);

    	extern void SetOffsetTime(const char * time);

        extern std::string GetDateStr(long long time = 0);

		extern Time::Date GetTimeDate(long long t = 0);

        // 获取毫秒时间戳
        extern long long NowMil();

        // 获取秒级时间戳
        extern long long NowSec();

    	extern long long NowSystemSec();

		//获取gmt时间
		extern std::string GetDateGMT(long long t = 0);

		extern std::string GetDateISO(long long t = 0);

		extern std::string GetDateDT(long long t = 0);

		// 获取时间字符串 精确到秒
        extern std::string GetDateString(long long t = 0);

    	extern std::string GetGMTDateString(long long t = 0);


        // 获取时间字符串精确到天
        extern std::string GetYearMonthDayString(long long t = 0);

		// 根据字符串获取时间戳
		extern long long GetTimeByString(const char * str);

    	//获取下一天 时间戳
    	extern long long GetNextDayTime();

    	//获取下一小时 时间戳
    	extern long long GetNextHourTime();

    	//获取下一分钟 时间戳
    	extern long long GetNextMinuteTime();

    	//获取新时间
        extern long long GetNewTime(int hour = 0, int minute = 0, int second = 0);

        // 获取十分秒
		extern void GetHourMinSecond(long long sec, int time[3]);

		extern void GetHourMinSecond(long long sec, int &hour, int &min, int &second);

		extern Time::Date CalcHourMinSecond(long long sec);
    }

	namespace HighTime
	{
		extern long long NowMil();
		extern long long NowSec();
	}
// namespace Helper::Time
}