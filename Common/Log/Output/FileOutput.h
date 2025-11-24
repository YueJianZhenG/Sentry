//
// Created by yy on 2023/8/12.
//

#ifndef APP_FILEOUTPUT_H
#define APP_FILEOUTPUT_H
#include"Log/Common/Logger.h"
#include<fstream>

namespace custom
{
	class FileOutput : public IOutput
	{
	public:
		explicit FileOutput(FileConfig path);
	private:
		void Close() final;
		void OnNewDay() final;
		bool Start(Asio::Context &io) final;
		void Push(Asio::Context &io, const std::string &name, const custom::LogInfo &logInfo) final;
	private:
		bool Init();
		bool OpenFile();
		void SwitchFile();
	private:
		size_t mIndex;
		std::string mPath; //当前打开玩家路径
		size_t mFileLine;	//当前文件大小
		FileConfig mConfig;
		std::ofstream mDevStream;
	};
}


#endif //APP_FILEOUTPUT_H
