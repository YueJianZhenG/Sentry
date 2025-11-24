//
// Created by yy on 2023/8/12.
//

#ifndef APP_LOGGERCOMPONENT_H
#define APP_LOGGERCOMPONENT_H

#include <mutex>
#include <unordered_map>
#include "Log/Common/Logger.h"
#include "Entity/Component/Component.h"

namespace acs
{
	class LoggerComponent final : public Component, public IDestroy
	{
	public:
		LoggerComponent();
		~LoggerComponent() final;
	public:
		void Flush();
		void DropAllLog();
		void Flush(const std::string & name);
		void PushLog(std::unique_ptr<custom::LogInfo>& logInfo);
		void PushLog(const std::string & name, std::unique_ptr<custom::LogInfo>& logInfo);
	private:
		bool Create(const custom::LogConfig & config);
		custom::Logger * GetLogger(const std::string& name);
		custom::Logger * GetLogger(custom::LogLevel logLevel);
	private:
		bool Awake() final;
		void OnDestroy() final;
		void OnNewDay(int days, int week);
	private:
		int mConsole;
		std::mutex mMutex;
		class ThreadComponent * mThread;
		std::vector<custom::LogConfig> mConfig;
		std::unique_ptr<custom::Logger> mAllLog;
		std::vector<std::unique_ptr<custom::Logger>> mNameLoggers;
		std::vector<std::unique_ptr<custom::Logger>> mLevelLoggers;
	};
}


#endif //APP_LOGGERCOMPONENT_H
