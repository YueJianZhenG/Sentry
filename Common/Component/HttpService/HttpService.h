//
// Created by yjz on 2022/1/22.
//

#ifndef SENTRY_HTTPSERVICE_H
#define SENTRY_HTTPSERVICE_H
#include"Component/Component.h"
#include"Method/MethodRegister.h"
namespace Sentry
{
	class HttpService : public Component, public IServiceBase
	{
	 public:
		HttpService() = default;
		virtual ~HttpService() = default;
	 protected:
		bool LoadService() final;
		bool IsStartComplete() final { return true; }
		virtual bool OnInitService(HttpServiceRegister & serviceRegister) = 0;
		bool IsStartService() final { return this->mServiceRegister != nullptr;}
	 public:
		std::shared_ptr<Json::Writer> Invoke(const std::string& name, std::shared_ptr<Json::Reader> request);
	 private:
		std::shared_ptr<HttpServiceRegister> mServiceRegister;
	};
}
#endif //SENTRY_HTTPSERVICE_H
