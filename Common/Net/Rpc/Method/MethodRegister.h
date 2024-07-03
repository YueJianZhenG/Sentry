//
// Created by mac on 2022/4/12.
//

#ifndef APP_METHODREGISTER_H
#define APP_METHODREGISTER_H

#include"ServiceMethod.h"
#include"Lua/Engine/Define.h"
#include"Http/Method/HttpServiceMethod.h"
#include"Entity/Component/Component.h"
namespace joke
{
	class ServiceMethodRegister
	{
	public:
		explicit ServiceMethodRegister(Component * component);
	public:
		template<typename T>
		bool Bind(std::string name, ServiceMethodType1<T> func, const std::string key)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod1<T>>(name, component, func, key));
		}

		template<typename T>
		bool Bind(std::string name, ServiceMethodType11<T> func, const std::string key)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod1<T>>(name, component, func, key));
		}

		template<typename T, typename T1>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType2<T, T1> func, const std::string key)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod2<T, T1>>(name,component, func, key));
		}

		template<typename T, typename T1>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType22<T, T1> func, const std::string key)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod2<T, T1>>(name, component, func, key));
		}

		template<typename T, typename T1, typename T2>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value
			&& std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType3<T, T1, T2> func, const std::string key)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod3<T, T1, T2>>(name, component, func, key));
		}

		template<typename T, typename T1, typename T2>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value
			&& std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType33<T, T1, T2> func, const std::string key)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod3<T, T1, T2>>(name, component, func, key));
		}

		template<typename T, typename T1>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType4<T, T1> func, const std::string key)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod4<T, T1>>(name, component, func, key));
		}

		template<typename T, typename T1>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType44<T, T1> func, const std::string key)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod4<T, T1>>(name, component, func, key));
		}

		template<typename T>
		bool Bind(std::string name, ServiceMethodType6<T> func, const std::string key)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod6<T>>(name, component, func, key));
		}

		template<typename T>
		bool Bind(std::string name, ServiceMethodType7<T> func, const std::string key)
		{
			T* component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod7<T>>(name, component, func, key));
		}

		template<typename T, typename T1>
		bool Bind(std::string name, ServiceMethodType34<T, T1> func, const std::string key)
		{
			T* component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_unique<ServiceMethod34<T, T1>>(name, component, func, key));
		}

	public:
		ServiceMethod* GetMethod(const std::string& name);
		bool AddMethod(std::unique_ptr<ServiceMethod> method);
	private:
		Component * mComponent;
		std::unordered_map<std::string, std::unique_ptr<ServiceMethod>> mMethodMap;
	};
}

namespace joke
{
	class HttpServiceRegister
	{
	 public:
		explicit HttpServiceRegister(Component * o) : mComponent(o) { }
	 public:
		template<typename T>
		bool Bind(const std::string& name, HttpMethod<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			T * component = this->mComponent->Cast<T>();
            std::unique_ptr<HttpServiceMethod> httpServiceMethod =
                    std::make_unique<CppHttpServiceMethod<T>>(name, component, std::move(func));
			return this->AddMethod(std::move(httpServiceMethod));
		}

		template<typename T>
		bool Bind(const std::string& name, HttpMethod2<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			T * component = this->mComponent->Cast<T>();
			std::unique_ptr<HttpServiceMethod> httpServiceMethod =
					std::make_unique<CppHttpServiceMethod3<T>>(name, component, std::move(func));
			return this->AddMethod(std::move(httpServiceMethod));
		}

        template<typename T>
        bool Bind(const std::string& name, HttpJsonMethod1<T> func)
        {
            auto iter = this->mHttpMethodMap.find(name);
            if (iter != this->mHttpMethodMap.end())
            {
                return false;
            }
            T * component = this->mComponent->Cast<T>();
            std::unique_ptr<HttpServiceMethod> httpServiceMethod =
                std::make_unique<JsonHttpServiceMethod1<T>>(name, component, std::move(func));
            return this->AddMethod(std::move(httpServiceMethod));
        }

		template<typename T>
		bool Bind(const std::string& name, HttpMethod4<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			T * component = this->mComponent->Cast<T>();
			std::unique_ptr<HttpServiceMethod> httpServiceMethod =
					std::make_unique<CppHttpServiceMethod2<T>>(name, component, std::move(func));
			return this->AddMethod(std::move(httpServiceMethod));
		}


        template<typename T>
        bool Bind(const std::string& name, HttpJsonMethod2<T> func)
        {
            auto iter = this->mHttpMethodMap.find(name);
            if (iter != this->mHttpMethodMap.end())
            {
                return false;
            }
            T * component = this->mComponent->Cast<T>();
            std::unique_ptr<HttpServiceMethod> httpServiceMethod =
                std::make_unique<JsonHttpServiceMethod2<T>>(name, component, std::move(func));
            return this->AddMethod(std::move(httpServiceMethod));
        }

        template<typename T>
        bool Bind(const std::string& name, HttpJsonMethod3<T> func)
        {
            auto iter = this->mHttpMethodMap.find(name);
            if (iter != this->mHttpMethodMap.end())
            {
                return false;
            }
            T * component = this->mComponent->Cast<T>();
            std::unique_ptr<HttpServiceMethod> httpServiceMethod =
                std::make_unique<JsonHttpServiceMethod3<T>>(name, component, std::move(func));
            return this->AddMethod(std::move(httpServiceMethod));
        }

		template<typename T>
		bool Bind(const std::string& name, HttpJsonMethod4<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			T * component = this->mComponent->Cast<T>();
			std::unique_ptr<HttpServiceMethod> httpServiceMethod =
					std::make_unique<JsonHttpServiceMethod4<T>>(name, component, std::move(func));
			return this->AddMethod(std::move(httpServiceMethod));
		}

		template<typename T>
		bool Bind(const std::string& name, HttpJsonMethod5<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			T * component = this->mComponent->Cast<T>();
			std::unique_ptr<HttpServiceMethod> httpServiceMethod =
					std::make_unique<JsonHttpServiceMethod5<T>>(name, component, std::move(func));
			return this->AddMethod(std::move(httpServiceMethod));
		}

		template<typename T>
		bool Bind(const std::string& name, HttpFromMethod1<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			T * component = this->mComponent->Cast<T>();
			std::unique_ptr<HttpServiceMethod> httpServiceMethod =
					std::make_unique<FromHttpServiceMethod1<T>>(name, component, std::move(func));
			return this->AddMethod(std::move(httpServiceMethod));
		}

		template<typename T>
		bool Bind(const std::string& name, HttpFromMethod2<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			T * component = this->mComponent->Cast<T>();
			std::unique_ptr<HttpServiceMethod> httpServiceMethod =
					std::make_unique<FromHttpServiceMethod2<T>>(name, component, std::move(func));
			return this->AddMethod(std::move(httpServiceMethod));
		}

		template<typename T>
		bool Bind(const std::string& name, HttpFromMethod3<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			T * component = this->mComponent->Cast<T>();
			std::unique_ptr<HttpServiceMethod> httpServiceMethod =
					std::make_unique<FromHttpServiceMethod3<T>>(name, component, std::move(func));
			return this->AddMethod(std::move(httpServiceMethod));
		}

		template<typename T>
		bool Bind(const std::string& name, HttpFromMethod4<T> func)
		{
			auto iter = this->mHttpMethodMap.find(name);
			if (iter != this->mHttpMethodMap.end())
			{
				return false;
			}
			T * component = this->mComponent->Cast<T>();
			std::unique_ptr<HttpServiceMethod> httpServiceMethod =
					std::make_unique<FromHttpServiceMethod4<T>>(name, component, std::move(func));
			return this->AddMethod(std::move(httpServiceMethod));
		}

	public:
		void Clear() { this->mHttpMethodMap.clear(); }
		HttpServiceMethod* GetMethod(const std::string& name);
        bool AddMethod(std::unique_ptr<HttpServiceMethod> method);
    private:
		Component * mComponent;
		std::unordered_map<std::string, std::unique_ptr<HttpServiceMethod>> mHttpMethodMap;
    };
}



#endif //APP_METHODREGISTER_H