
#pragma once
#include "Http/Service/HttpService.h"


namespace acs
{
	class Pay7xfService : public HttpService
	{
	public:
		Pay7xfService();
	private:
		bool OnInit() final;
	private:
		int Create(const http::Request & request, http::Response & response);
		int OnReturn(const http::FromContent & request, http::Response & response);
		int OnNotify(const http::FromContent & request, http::Response & response);
	private:
		class Pay7xfComponent * mPayComponent;
	};
}