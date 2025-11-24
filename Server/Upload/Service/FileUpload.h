//
// Created by leyi on 2024/3/28.
//

#ifndef APP_FILEUPLOAD_H
#define APP_FILEUPLOAD_H
#include "Http/Service/HttpService.h"

namespace acs
{
	class FileUpload final : public HttpService
	{
	public:
		FileUpload();
		~FileUpload() final = default;
	private:
		bool Awake() final;
		bool OnInit() final;
	private:
		int File(const http::Request & request, http::Response & response);
		int Proxy(const http::Request & request, http::Response & response);
		int Oss(const http::FromContent & request, json::w::Document & response);
	private:
		std::string mDoMain;
		class AliOssComponent * mOss;
	};
}


#endif //APP_FILEUPLOAD_H
