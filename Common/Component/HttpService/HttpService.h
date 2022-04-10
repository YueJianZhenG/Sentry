//
// Created by yjz on 2022/1/22.
//

#ifndef SENTRY_HTTPSERVICE_H
#define SENTRY_HTTPSERVICE_H
#include"Component/Component.h"
#include"Method/HttpServiceMethod.h"
namespace Sentry
{
	class HttpService : public Component
	{
	 public:
		HttpService() = default;
		virtual ~HttpService() = default;

	 public:
		std::shared_ptr<Json::Writer> Invoke(const std::string& name, std::shared_ptr<Json::Reader> request);
	 public:
		template<typename T>
		bool Bind(const std::string& name, HttpJsonMethod1<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			this->mHttpMethodMap.emplace(name, std::make_shared<
				HttpServiceJsonMethod1<T>>((T*)this, std::move(func)));
			return true;
		}

		template<typename T>
		bool Bind(const std::string& name, HttpJsonMethod2<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			this->mHttpMethodMap.emplace(name, std::make_shared<
				HttpServiceJsonMethod2<T>>((T*)this, std::move(func)));
			return true;
		}

	 private:
		std::unordered_map<std::string, std::shared_ptr<HttpServiceMethod>> mHttpMethodMap;
	};
#define BIND_HTTP_FUNCTION(func) LOG_CHECK_RET_FALSE(this->Bind(GetFunctionName(#func), &func))
}
#endif //SENTRY_HTTPSERVICE_H