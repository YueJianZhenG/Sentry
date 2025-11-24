//
// Created by yy on 2023/8/12.
//
#include "fmt.h"
#include "FileOutput.h"
#include "Util/Tools/TimeHelper.h"
#include "Util/File/FileHelper.h"
#include "Util/File/DirectoryHelper.h"
#include "Log/Common/CommonLogDef.h"

namespace custom
{
	FileOutput::FileOutput(FileConfig config)
		: mConfig(std::move(config))
	{
		this->mIndex = 0;
		this->mFileLine = 0;
	}

	void FileOutput::Close()
	{
		if (this->mDevStream.is_open())
		{
			this->mDevStream.flush();
			this->mDevStream.close();
		}
	}

	bool FileOutput::Init()
	{
		this->mIndex = 0;
		this->mFileLine = 0;
		const std::string& root = this->mConfig.Root;
		std::string time = help::Time::GetYearMonthDayString();
		const std::string dir = fmt::format("{}/{}", root, time);
		if (!help::dir::MakeDir(dir))
		{
			return false;
		}
		this->mPath = fmt::format("{}/{}.log", dir, this->mConfig.Name);
		return true;
	}

	bool FileOutput::Start(Asio::Context& io)
	{
		return this->Init();
	}

	void FileOutput::OnNewDay()
	{
		this->Init();
		this->OpenFile();
	}


	bool FileOutput::OpenFile()
	{
		if (!this->mDevStream.is_open())
		{
			this->mDevStream.open(this->mPath, std::ios::out | std::ios::app | std::ios::ate);
			if (!this->mDevStream.is_open())
			{
				return false;
			}
			this->mFileLine = 0;
			help::fs::GetFileLine(this->mPath, this->mFileLine);
		}
		return true;
	}

	void FileOutput::Push(Asio::Context& io, const std::string& name, const custom::LogInfo& logData)
	{
		if (!this->OpenFile())
		{
			return;
		}
		constexpr char empty = ' ';
		std::string time = help::Time::GetDateString();
		switch (logData.Level)
		{
		case LogLevel::Debug:
			{
				static std::string type = fmt::format(" [{:<7}] ", "debug");
				this->mDevStream.write(time.c_str(), (std::streamsize)time.size()) << empty;
				this->mDevStream.write(type.c_str(), (std::streamsize)type.size());
				break;
			}

		case LogLevel::Info:
			{
				static std::string type = fmt::format(" [{:<7}] ", "info");
				this->mDevStream.write(time.c_str(), (std::streamsize)time.size()) << empty;
				this->mDevStream.write(type.c_str(), (std::streamsize)type.size());
				break;
			}

		case LogLevel::Warn:
			{
				static std::string type = fmt::format(" [{:<7}] ", "warn");
				this->mDevStream.write(time.c_str(), (std::streamsize)time.size()) << empty;
				this->mDevStream.write(type.c_str(), (std::streamsize)type.size());
				break;
			}

		case LogLevel::Error:
			{
				static std::string type = fmt::format(" [{:<7}] ", "error");
				this->mDevStream.write(time.c_str(), (std::streamsize)time.size()) << empty;
				this->mDevStream.write(type.c_str(), (std::streamsize)type.size());
				break;
			}

		case LogLevel::Fatal:
			{
				static std::string type = fmt::format(" [{:<7}] ", "fatal");
				this->mDevStream.write(time.c_str(), (std::streamsize)time.size()) << empty;
				this->mDevStream.write(type.c_str(), (std::streamsize)type.size());
				break;
			}
		default:
			{
				std::string type = fmt::format(" [{:<7}] ", name);
				this->mDevStream.write(time.c_str(), (std::streamsize)time.size()) << empty;
				this->mDevStream.write(type.c_str(), (std::streamsize)type.size());
			}
			break;
		}
		if (!logData.File.empty())
		{
			this->mDevStream.write(logData.File.c_str(), (std::streamsize)logData.File.size());
			this->mDevStream << empty;
		}

		this->mDevStream.write(logData.Content.c_str(), (std::streamsize)logData.Content.size());

		this->mDevStream << "\n";
		if (logData.Stack != nullptr)
		{
			this->mFileLine++;
			this->mDevStream.write(logData.Stack->c_str(), (std::streamsize)logData.Stack->size()) << "\n";
		}
		this->mFileLine++;
		if (this->mFileLine >= this->mConfig.MaxLine)
		{
			this->SwitchFile();
			return;
		}
		size_t fileSize = this->mDevStream.tellp();
		if (fileSize >= this->mConfig.MaxSize)
		{
			this->SwitchFile();
			return;
		}
		this->mDevStream.flush();
	}

	void FileOutput::SwitchFile()
	{
		this->mIndex++;
		this->mFileLine = 0;
		this->mDevStream.flush();
		this->mDevStream.close();
		const std::string& root = this->mConfig.Root;
		std::string time = help::Time::GetYearMonthDayString();

		std::string dir = fmt::format("{}/{}", root, time);
		if (!help::dir::DirectorIsExist(dir))
		{
			this->mIndex = 0;
			help::dir::MakeDir(dir);
		}
		std::string path = fmt::format("{}/{}-{}.log", dir, this->mConfig.Name, this->mIndex);

		while (help::fs::FileIsExist(path.c_str()))
		{
			this->mIndex++;
			path = fmt::format("{}/{}-{}.log", dir, this->mConfig.Name, this->mIndex);
		}
		std::rename(this->mPath.c_str(), path.c_str());
		this->OpenFile();
	}
}
