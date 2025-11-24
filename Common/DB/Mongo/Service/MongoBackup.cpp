//
// Created by leyi on 2023/11/6.
//

#include <ostream>
#include "MongoBackup.h"
#include "Util/Zip/Zip.h"
#include "Core/System/System.h"
#include "Node/Actor/Node.h"
#include "Server/Config/ServerConfig.h"
#include "Util/File/DirectoryHelper.h"
#include "Util/File/FileHelper.h"
#include "AliCloud/Component/AliOssComponent.h"
#include "Async/Component/CoroutineComponent.h"
#include "Mongo/Component/MongoDBComponent.h"
#include "Util/Tools/TimeHelper.h"
#include "Server/Component/ThreadComponent.h"
#include "Async/Source/TaskSource.h"
#include "Entity/Actor/App.h"
namespace acs
{
	MongoBackup::MongoBackup()
	{
		this->mMongo = nullptr;
		REGISTER_JSON_CLASS_MUST_FIELD(mongo::BackupConfig, oss);
		REGISTER_JSON_CLASS_MUST_FIELD(mongo::BackupConfig, path);
		REGISTER_JSON_CLASS_MUST_FIELD(mongo::BackupConfig, batchSize);
		REGISTER_JSON_CLASS_MUST_FIELD(mongo::BackupConfig, collections);
	}

	bool MongoBackup::Awake()
	{
		this->mConfig.batchSize = 500;
		return ServerConfig::Inst()->Get("mongo.backup", this->mConfig);
	}

	bool MongoBackup::OnInit()
	{
		BIND_COMMON_HTTP_METHOD(MongoBackup::Backup);
		BIND_COMMON_HTTP_METHOD(MongoBackup::Recover);
		BIND_COMMON_HTTP_METHOD(MongoBackup::Upload);
		LOG_CHECK_RET_FALSE(this->mMongo = this->GetComponent<MongoDBComponent>())
		return true;
	}

	int MongoBackup::Upload(const http::FromContent& request, json::w::Document& response)
	{
		std::string name;
		LOG_ERROR_CHECK_ARGS(request.Get("name", name))
		std::string file = fmt::format("{}.zip", name);
		std::string path = fmt::format("{}/{}", this->mConfig.path, file);
		if(!help::fs::FileIsExist(path.c_str()))
		{
			return XCode::Failure;
		}
		AliOssComponent* oss = this->GetComponent<AliOssComponent>();
		if (oss == nullptr)
		{
			return XCode::Failure;
		}
		std::unique_ptr<oss::Response> ossResponse = oss->Upload(this->mConfig.oss, path, "backup");
		if (ossResponse == nullptr)
		{
			return XCode::Failure;
		}
		if(ossResponse->code == HttpStatus::OK)
		{
			std::remove(path.c_str());
		}
		std::unique_ptr<json::w::Value> jsonValue =  response.AddObject("data");
		{
			jsonValue->Add("url", ossResponse->url);
			jsonValue->Add("message", ossResponse->data);
			jsonValue->Add("status", ossResponse->code);
			jsonValue->Add("status_text", HttpStatusToString(ossResponse->code));

		}
		return XCode::Ok;
	}

	int MongoBackup::Backup(const http::FromContent& request, json::w::Document& response)
	{
		std::string name;
		if(!request.Get("name", name))
		{
			name = help::Time::GetYearMonthDayString();
		}
		std::string dir = fmt::format("{}/{}", this->mConfig.path, name);
		if(!help::dir::DirectorIsExist(dir))
		{
			help::dir::MakeDir(dir);
		}
		long long t1 = help::Time::NowMil();
#ifdef __OS_LINUX__
		std::string uri;
		if(!this->mMongo->FormatUri(uri))
		{
			return XCode::Failure;
		}
		//mongodump --uri=mongodb://127.0.0.1:27017/yjz --out=./backup
		std::string cmd = fmt::format("mongodump --uri={} --out={}", uri, dir);
		{
			std::list<std::string> output;
			if(!os::System::Run(cmd, output))
			{
				return XCode::Failure;
			}
			response.Add("result", output);
		}
#else
		unsigned int doneCount = 0;
		std::unique_ptr<json::w::Value> jsonValue = response.AddObject("data");
		for(const std::string & tab : this->mConfig.collections)
		{
			std::ofstream ofs;
			std::string path = fmt::format("{}/{}.json", dir, tab);
			ofs.open(path, std::ios::ate | std::ios::out | std::ios::binary);
			if (!ofs.is_open())
			{
				return XCode::Failure;
			}
			unsigned int count = 0;
			std::string jsonString;
			std::unique_ptr<bson::r::Document> document1;
			std::unique_ptr<mongo::Request> mongoRequest = std::make_unique<mongo::Request>();
			{
				bson::w::Document document;
				mongoRequest->GetCollection("find", tab);
			}
			std::unique_ptr<mongo::Response> mongoResponse = this->mMongo->Run(mongoRequest);
			while(mongoResponse != nullptr && !mongoResponse->IsEmpty())
			{
				for (const std::string& json: mongoResponse->result)
				{
					count++;
					doneCount++;
					ofs.write(json.c_str(), (int)json.size()) << "\n";
				}
				if (mongoResponse->GetCursor() == 0)
				{
					break;
				}
				mongoRequest = std::make_unique<mongo::Request>();
				{
					mongoRequest->cmd = "getMore";
					mongoRequest->document.Add("getMore", mongoResponse->GetCursor());
					mongoRequest->document.Add("collection", tab);
					if(this->mConfig.batchSize > 0)
					{
						mongoRequest->document.Add("batchSize", this->mConfig.batchSize);
					}
				}
				mongoResponse = this->mMongo->Run(mongoRequest);
			}
			jsonValue->Add(tab.c_str(), count);
			ofs.close();
		}
#endif
		long long t2 = help::Time::NowMil();
		Asio::Context & main = this->mApp->GetContext();
		std::string zipPath = fmt::format("{}/{}.zip", this->mConfig.path, name);
		Asio::Context & context  = this->GetComponent<ThreadComponent>()->GetContext();
		std::unique_ptr<TaskSource<bool>> waitTaskSource = std::make_unique<TaskSource<bool>>();
		asio::post(context, [&main, dir, zipPath, taskSource = waitTaskSource.get()] ()
		{
			bool result = false;
			if(help::zip::Create(dir, zipPath))
			{
				help::dir::RemoveAllFile(dir);
			}
			asio::post(main, [result, taskSource] { taskSource->SetResult(result); });
		});

		waitTaskSource->Await();
		long long t3 = help::Time::NowMil();
		std::unique_ptr<json::w::Value> timeObject = response.AddObject("time");
		{
			timeObject->Add("backup", fmt::format("{}ms", t2 - t1));
			timeObject->Add("zip", fmt::format("{}ms", t3 - t2));
		}
		return XCode::Ok;
	}

	int MongoBackup::Recover(const http::FromContent& request, json::w::Document& response)
	{
		std::string name, db;
		std::vector<std::string> filePaths;
		std::vector<std::string> fileLines;
		LOG_ERROR_CHECK_ARGS(request.Get("db", db))
		LOG_ERROR_CHECK_ARGS(request.Get("name", name))
		std::string outDir = fmt::format("{}/{}", this->mConfig.path, name);
		std::string zipPath = fmt::format("{}/{}.zip", this->mConfig.path, name);
		if(!help::fs::FileIsExist(zipPath))
		{
			AliOssComponent* oss = this->GetComponent<AliOssComponent>();
			if(oss == nullptr)
			{
				return XCode::Failure;
			}
			std::string objectKey = fmt::format("backup/{}.zip", name);
			if(!oss->Download(this->mConfig.oss, objectKey, zipPath))
			{
				std::remove(zipPath.c_str());
				return XCode::Failure;
			}
		}
		help::zip::Unzip(outDir, zipPath);
		help::dir::GetFilePaths(outDir, filePaths);
		std::unique_ptr<json::w::Value> jsonValue = response.AddObject("data");
		{
			for (const std::string& filePath: filePaths)
			{
				int sumCount = 0;
				fileLines.clear();
				std::string table;
				help::fs::GetFileName(filePath, table);
				help::fs::ReadTxtFile(filePath, fileLines);
				std::string targetTable = fmt::format("{}.{}", db, table);
				for (size_t index = 0; index < fileLines.size(); index += 100)
				{
					std::unique_ptr<mongo::Request> mongoRequest = std::make_unique<mongo::Request>();
					{
						bson::w::Document documents(false);
						for (size_t x = index; x < index + 100 && x < fileLines.size(); x++)
						{
							bson::w::Document document;
							if (!document.FromByJson(fileLines[x]))
							{
								return XCode::Failure;
							}
							documents.Push(document);
						}
						mongoRequest->GetCollection("insert", targetTable).Insert(documents);
					}
					std::unique_ptr<mongo::Response> mongoResponse = this->mMongo->Run(mongoRequest);
					if (mongoResponse)
					{
						int count = 0;
						if (mongoResponse->document.Get("n", count))
						{
							sumCount += count;
						}
					}
				}
				jsonValue->Add(targetTable.c_str(), sumCount);
			}
		}
		help::dir::RemoveAllFile(outDir);
		return XCode::Ok;
	}
}