﻿#pragma once

#include "ServiceBase.h"
#include <NetWork/ServiceMethod.h>

namespace Sentry
{

	inline std::string GetFunctionName(const std::string func)
	{
		size_t pos = func.find("::");
		return func.substr(pos + 2);
	}

    class LocalService : public ServiceBase
    {
    public:
        LocalService();

        virtual ~LocalService() {}

    public:
		bool AddMethod(ServiceMethod * method) final;
        bool HasMethod(const std::string &action) final;
		ServiceMethod * GetMethod(const std::string &method) final;
		const std::string &GetServiceName()final { return this->GetTypeName(); }
	protected:
		template<typename T>
		bool Bind(std::string name, ServiceMethodType1<T> func) {
			return this->Bind(new ServiceMethod1<T>(name, (T*)this, func));
		}

		template<typename T, typename T1>
		bool Bind(std::string name, ServiceMethodType2<T, T1> func) {
			return this->Bind(new ServiceMethod2<T, T1>(name, (T*)this, func));
		}

		template<typename T, typename T1, typename T2>
		bool Bind(std::string name, ServiceMethodType3<T, T1, T2> func) {
			return this->Bind(new ServiceMethod3<T, T1, T2>(name, (T*)this, func));
		}

		template<typename T, typename T1>
		bool Bind(std::string name, ServiceMethodType4<T, T1> func) {
			return this->Bind(new ServiceMethod4<T, T1>(name, (T*)this, func));
		}
    };
#define __ADD_SERVICE_METHOD__(func) SayNoAssertRetFalse_F(this->Bind(GetFunctionName(#func), &func))
}// namespace Sentry