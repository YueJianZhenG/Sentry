#include"LoggerComponent.h"
#include"App/App.h"
#include"spdlog/async.h"
#ifdef __DEBUG__
#include"Log/Debug.h"
#endif
#include"System/System.h"
#include"spdlog/sinks/rotating_file_sink.h"

namespace Sentry
{
	// 单线程  st  多线程  mt
	bool LoggerComponent::Awake()
	{
		this->mLogSaveTime = 3;
        this->mServerName = ServerConfig::Inst()->Name();
        this->mLogSavePath = fmt::format("{0}/log", System::WorkPath());
		this->CreateLogFile();
        return true;
	}

	void LoggerComponent::OnZeroRefresh()
	{
		this->CreateLogFile();
	}

	void LoggerComponent::OnDestory()
	{
		this->SaveAllLog();
	}

	void LoggerComponent::SaveAllLog()
	{
#ifdef __DEBUG__
		this->mAllLog->flush();
#else
		this->mInfoLog->flush();
		this->mDebugLog->flush();
		this->mErrorLog->flush();
		this->mFatalLog->flush();
		this->mWarningLog->flush();
#endif
		spdlog::drop_all();
	}

	void LoggerComponent::AddLog(spdlog::level::level_enum type, const std::string& log)
    {
#ifdef __DEBUG__
        this->mAllLog->log(type, log);
#endif
        switch (type)
        {
			case spdlog::level::level_enum::debug:
#ifndef __DEBUG__
                this->mDebugLog->debug(log);
#endif
			break;
            case spdlog::level::level_enum::info:
#ifndef __DEBUG__
                this->mInfoLog->info(log);
#endif
                break;
            case spdlog::level::level_enum::warn:
#ifndef __DEBUG__
                this->mWarningLog->warn(log);
#endif
                break;
            case spdlog::level::level_enum::err:
#ifndef __DEBUG__
                this->mErrorLog->error(log);
#endif
                break;
            case spdlog::level::level_enum::critical:
#ifndef __DEBUG__
                this->mFatalLog->critical(log);
#endif
                break;
            default:
                break;
        }
    }
#ifdef __ENABLE_START_LOG__
    void LoggerComponent::AddStartLog(spdlog::level::level_enum type, const std::string &log)
    {
        if(this->mStartLog != nullptr)
        {
            this->mStartLog->log(type, log);
            this->mStartLog->flush();
        }
    }

    void LoggerComponent::CloseStartLog()
    {
        if(this->mStartLog != nullptr)
        {
            this->mStartLog->flush();
            this->mStartLog = nullptr;
        }
    }
#endif
	void LoggerComponent::CreateLogFile()
	{
		spdlog::shutdown();
        spdlog::set_level(spdlog::level::debug);
        const std::string & name = ServerConfig::Inst()->Name();
        spdlog::flush_every(std::chrono::seconds(this->mLogSaveTime));
		std::string logPath = fmt::format("{0}/{1}/{2}", this->mLogSavePath,
			Helper::Time::GetYearMonthDayString(), this->mServerName);
#ifndef ONLY_MAIN_THREAD
#ifdef __DEBUG__
		spdlog::set_level(spdlog::level::level_enum::debug);
		this->mAllLog = spdlog::rotating_logger_st<spdlog::async_factory>("All", logPath + "/all.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
#else
		this->mInfoLog = spdlog::rotating_logger_mt<spdlog::async_factory>("Info", logPath + "/info.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
		this->mDebugLog = spdlog::rotating_logger_mt<spdlog::async_factory>("Debug", logPath + "/debug.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
		this->mFatalLog = spdlog::rotating_logger_mt<spdlog::async_factory>("Fatal", logPath + "/fatal.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
		this->mErrorLog = spdlog::rotating_logger_mt<spdlog::async_factory>("Error", logPath + "/error.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
		this->mWarningLog = spdlog::rotating_logger_mt<spdlog::async_factory>("Warning", logPath + "/warning.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
#endif
#else
#ifdef __DEBUG__
		spdlog::set_level(spdlog::level::level_enum::debug);
		this->mAllLog = spdlog::rotating_logger_st<spdlog::async_factory>(name,
			logPath + "/all.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
#else
		this->mInfoLog = spdlog::rotating_logger_st<spdlog::async_factory>("Info", logPath + "/info.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
		this->mDebugLog = spdlog::rotating_logger_st<spdlog::async_factory>("Debug", logPath + "/debug.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
		this->mFatalLog = spdlog::rotating_logger_st<spdlog::async_factory>("Fatal", logPath + "/fatal.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
		this->mErrorLog = spdlog::rotating_logger_st<spdlog::async_factory>("Error", logPath + "/error.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
		this->mRecordLog = spdlog::rotating_logger_st<spdlog::async_factory>("Record", logPath + "/record.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
		this->mWarningLog = spdlog::rotating_logger_st<spdlog::async_factory>("Warning", logPath + "/warning.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
#endif

#endif

#ifdef __ENABLE_START_LOG__
        std::string logName = fmt::format("{0}{1}", "Start", name);
        this->mStartLog = spdlog::rotating_logger_mt<spdlog::async_factory>(
                logName, "./log/start.log", LOG_FILE_MAX_SIZE, LOG_FILE_MAX_SUM);
#endif
	}
}