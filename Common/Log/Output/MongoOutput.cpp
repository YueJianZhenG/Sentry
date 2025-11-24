//
// Created by yy on 2023/8/13.
//

#include"MongoOutput.h"
#include"Util/Tools/TimeHelper.h"
#include"Mongo/Client/MongoFactory.h"

namespace custom
{
	MongoOutput::MongoOutput(mongo::Config config)
		: mConfig(std::move(config))
	{

	}

	bool MongoOutput::Start(Asio::Context& io)
	{
		std::unique_ptr<tcp::Socket> socket = std::make_unique<tcp::Socket>(io);
		{
			socket->Init(this->mConfig.address);
			this->mMonClient = std::make_shared<mongo::Client>(1, this->mConfig, io);
		}
		return this->mMonClient->Start(socket.release());
	}

	void MongoOutput::Push(Asio::Context &io, const std::string& name, const custom::LogInfo& logInfo)
	{
		std::string time = help::Time::GetDateString();

		bson::w::Document document;
		if(!logInfo.File.empty())
		{
			document.Add("file", logInfo.File);
		}
		if(logInfo.Stack != nullptr)
		{
			document.Add("stack", logInfo.Stack->c_str(), logInfo.Stack->size());
		}
		document.Add("time", time);
		if(!document.FromByJson(logInfo.Content))
		{
			document.Add("log", logInfo.Content);
		}

		switch(logInfo.Level)
		{
			case custom::LogLevel::Info:
				document.Add("level", "info");
				break;
			case custom::LogLevel::Debug:
				document.Add("level", "debug");
				break;
			case custom::LogLevel::Warn:
				document.Add("level", "warn");
				break;
			case custom::LogLevel::Error:
				document.Add("level", "error");
				break;
			case custom::LogLevel::Fatal:
				document.Add("level", "fatal");
				break;
			default:
				document.Add("level", name);
		}
		document.Add("t", help::Time::NowSec());
		std::string table = help::Time::GetYearMonthDayString();
		std::unique_ptr<mongo::Request> request = mongo::MongoFactory::Insert(table, document);
		{
			request->dataBase = this->mConfig.db;
		}
		mongo::Response response("insert");
		this->mMonClient->SyncSend(request, response);
	}
}