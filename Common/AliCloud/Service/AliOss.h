//
// Created by 64658 on 2025/8/7.
//

#pragma once
#include "Http/Service/HttpService.h"
#include "AliCloud/Component/AliOssComponent.h"

namespace acs
{
	class AliOss : public HttpService
	{
	public:
		AliOss();
	private:
		bool OnInit() final;
	private:
		int Upload(const http::Request & request, http::Response & response);
		int Url(const http::FromContent & request, http::Response & response);
		int Make(const http::FromContent & request, json::w::Document & response);
	private:
		class AliOssComponent * mAliOss;
	};

}



