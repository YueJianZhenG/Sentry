//
// Created by yy on 2023/8/12.
//
#include "Lua/Lib/Lib.h"
#include "LoggerComponent.h"
#include "Entity/Actor/App.h"
#include "Core/System/System.h"
#include "Log/Output/FileOutput.h"
#ifdef __CONSOLE_LOG__
#include "Log/Output/ConsoleOutput.h"
#endif
#include "Log/Output/WeChatOutput.h"
#include "Log/Output/MongoOutput.h"

#ifdef __ENABLE_DING_DING_PUSH
#include "Http/Common/HttpRequest.h"
#include "Http/Common/HttpResponse.h"
#include "Http/Component/HttpComponent.h"
#endif
#include "Http/Component/NotifyComponent.h"
#include "Server/Component/ThreadComponent.h"

namespace acs
{
	LoggerComponent::LoggerComponent()
	{
		this->mConsole = 0;
		this->mThread = nullptr;
		this->mAllLog = nullptr;
		custom::LogConfig::RegisterField("wx", &custom::LogConfig::wx);
		custom::LogConfig::RegisterField("open", &custom::LogConfig::open);
		custom::LogConfig::RegisterField("ding", &custom::LogConfig::ding);
		custom::LogConfig::RegisterField("name", &custom::LogConfig::name);
		custom::LogConfig::RegisterField("path", &custom::LogConfig::path);
		custom::LogConfig::RegisterField("mongo", &custom::LogConfig::mongo);
		custom::LogConfig::RegisterField("level", &custom::LogConfig::levelName);
		custom::LogConfig::RegisterField("max_line", &custom::LogConfig::max_line);
		custom::LogConfig::RegisterField("max_size", &custom::LogConfig::max_size);
	}

	LoggerComponent::~LoggerComponent()
	{
		this->mNameLoggers.clear();
		this->mLevelLoggers.clear();
	}

	bool LoggerComponent::Awake()
	{
		std::string name;
		os::System::GetAppEnv("name", name);
		std::unique_ptr<json::r::Value> jsonArray;
		LuaCCModuleRegister::Add([](Lua::CCModule& ccModule)
		{
			ccModule.Open("core.log", lua::lib::luaopen_llog);
		});
		os::System::GetAppEnv("console", this->mConsole);
		this->mThread = this->GetComponent<ThreadComponent>();
		this->mLevelLoggers.resize((size_t)custom::LogLevel::All);
		std::unordered_map<std::string, custom::LogLevel> logLevelMap = {
				{"all", custom::LogLevel::All, },
				{"none", custom::LogLevel::None, },
				{ "debug", custom::LogLevel::Debug, },
				{ "info",  custom::LogLevel::Info, },
				{ "warn",  custom::LogLevel::Warn, },
				{ "error", custom::LogLevel::Error, },
				{ "fatal", custom::LogLevel::Fatal, },
		};
		this->mApp->Config().Get("log", this->mConfig);
		for (custom::LogConfig& config: this->mConfig)
		{
			auto iter = logLevelMap.find(config.levelName);
			if (iter == logLevelMap.end())
			{
				return false;
			}
			config.level = iter->second;
			if (config.open && !this->Create(config))
			{
				return false;
			}
		}
		help::OnNewDayEvent::Add(this, &LoggerComponent::OnNewDay);
		return true;
	}

	void LoggerComponent::OnNewDay(int days, int week)
	{
		std::lock_guard<std::mutex> lock(this->mMutex);
		for(std::unique_ptr<custom::Logger> & logger : this->mNameLoggers)
		{
			logger->OnNewDay();
		}
		for(std::unique_ptr<custom::Logger> & logger : this->mLevelLoggers)
		{
			if(logger != nullptr)
			{
				logger->OnNewDay();
			}
		}
	}


	void LoggerComponent::OnDestroy()
	{
		this->DropAllLog();
		std::this_thread::sleep_for(std::chrono::seconds(1)); //等待日志保存
		{
			this->mNameLoggers.clear();
			this->mLevelLoggers.clear();
		}
	}

	void LoggerComponent::Flush()
	{
		std::lock_guard<std::mutex> lock(this->mMutex);
		for(std::unique_ptr<custom::Logger> & logger : this->mNameLoggers)
		{
			logger->Flush();
		}

		for(std::unique_ptr<custom::Logger> & logger : this->mLevelLoggers)
		{
			if(logger)
			{
				logger->Flush();
			}
		}
	}

	void LoggerComponent::DropAllLog()
	{
		std::lock_guard<std::mutex> lock(this->mMutex);

		for(std::unique_ptr<custom::Logger> & logger : this->mNameLoggers)
		{
			logger->Close();
		}

		for(std::unique_ptr<custom::Logger> & logger : this->mLevelLoggers)
		{
			if(logger)
			{
				logger->Close();
			}
		}
	}

	void LoggerComponent::Flush(const std::string& name)
	{
		std::lock_guard<std::mutex> lock(this->mMutex);
		custom::Logger * logger = this->GetLogger(name);
		if(logger != nullptr)
		{
			logger->Flush();
		}

	}

	void LoggerComponent::PushLog(std::unique_ptr<custom::LogInfo>& log)
	{
		if(this->mConsole == 1)
		{
			Debug::Console(*log);
		}
		custom::Logger* logger = this->GetLogger(log->Level);
		if (logger != nullptr)
		{
			logger->Push(log);
		}
	}

	void LoggerComponent::PushLog(const std::string &name, std::unique_ptr<custom::LogInfo>& logInfo)
	{
		if(this->mConsole == 1)
		{
			Debug::Console(name, *logInfo);
		}
		custom::Logger * logger = this->GetLogger(name);
		if(logger != nullptr)
		{
			logger->Push(name, logInfo);
		}
	}

	custom::Logger *LoggerComponent::GetLogger(const std::string &name)
	{
		std::lock_guard<std::mutex> lock(this->mMutex);
		auto iter = std::find_if(this->mNameLoggers.begin(), this->mNameLoggers.end(),
			[&name](std::unique_ptr<custom::Logger> & log1) {
			return log1->GetName() == name;
		});
		if(iter != this->mNameLoggers.end())
		{
			return (*iter).get();
		}
		return this->mAllLog.get();
	}

	custom::Logger* LoggerComponent::GetLogger(custom::LogLevel logLevel)
	{
		std::lock_guard<std::mutex> lock(this->mMutex);
		if(logLevel > custom::LogLevel::None && logLevel < custom::LogLevel::All)
		{
			std::unique_ptr<custom::Logger> & levelLog = this->mLevelLoggers[(size_t)logLevel];
			if(levelLog != nullptr)
			{
				return levelLog.get();
			}
		}
		return this->mAllLog.get();
	}

	bool LoggerComponent::Create(const custom::LogConfig & config)
	{
		Asio::Context & context = this->mThread->GetContext();
		std::unique_ptr<custom::Logger> logger = std::make_unique<custom::Logger>(config.name, context);
		{
			if (!config.path.empty())
			{
				custom::FileConfig fileConfig;
				fileConfig.Name = config.name;
				fileConfig.MaxLine = config.max_line;
				fileConfig.MaxSize = config.max_size;
				const std::string & server = this->mApp->Name();
				fileConfig.Root = fmt::format("{}/{}",config.path, server) ;
				logger->AddOutput<custom::FileOutput>(fileConfig);
			}
			if (!config.wx.empty())
			{
				//微信通知组件
				http::Url url;
				if(!url.Decode(config.wx))
				{
					return false;
				}
				logger->AddOutput<custom::WeChatOutput>(config.wx);
			}
			mongo::Config mongoConfig;
			if (!config.mongo.empty() && mongoConfig.Decode(config.mongo))
			{
				mongoConfig.Get("db", mongoConfig.db);
				mongoConfig.Get("user", mongoConfig.user);
				mongoConfig.Get("address", mongoConfig.address);
				mongoConfig.Get("password", mongoConfig.password);
				if(!mongoConfig.Get("mechanism", mongoConfig.mechanism))
				{
					mongoConfig.mechanism = mongo::auth::SCRAM_SHA1;
				}
#ifdef __ENABLE_OPEN_SSL__
				assert(mongoConfig.mechanism == mongo::auth::SCRAM_SHA1
					   || mongoConfig.mechanism == mongo::auth::SCRAM_SHA256);
#else
				assert(mongoConfig.mechanism == mongo::auth::SCRAM_SHA1);
#endif
				logger->AddOutput<custom::MongoOutput>(mongoConfig);
			}
		}
		if (!logger->Start())
		{
			return false;
		}
		switch(config.level)
		{
		case custom::LogLevel::All:
			this->mAllLog = std::move(logger);
			break;
		case custom::LogLevel::None:
			this->mNameLoggers.emplace_back(std::move(logger));
			break;
		default:
			this->mLevelLoggers[(size_t)config.level] = std::move(logger);
			break;
		}
		return true;
	}
}