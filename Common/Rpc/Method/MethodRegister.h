//
// Created by mac on 2022/4/12.
//

#ifndef SERVER_METHODREGISTER_H
#define SERVER_METHODREGISTER_H

#include"ServiceMethod.h"
#include"Lua/Engine/Define.h"
#include"Http/Method/HttpServiceMethod.h"
#include"Entity/Component/Component.h"
namespace Tendo
{
	class ServiceMethodRegister
	{
	public:
		explicit ServiceMethodRegister(Component * component);
	public:
		template<typename T>
		bool Bind(std::string name, ServiceMethodType1<T> func)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod1<T>>(name, component, func));
		}

		template<typename T>
		bool Bind(std::string name, ServiceMethodType11<T> func)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod1<T>>(name, component, func));
		}

		template<typename T, typename T1>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType2<T, T1> func)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod2<T, T1>>(name,component, func));
		}

		template<typename T, typename T1>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType22<T, T1> func)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod2<T, T1>>(name, component, func));
		}

		template<typename T, typename T1, typename T2>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value
			&& std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType3<T, T1, T2> func)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod3<T, T1, T2>>(name, component, func));
		}

		template<typename T, typename T1, typename T2>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value
			&& std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType33<T, T1, T2> func)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod3<T, T1, T2>>(name, component, func));
		}

		template<typename T, typename T1>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType4<T, T1> func)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod4<T, T1>>(name, component, func));
		}

		template<typename T, typename T1>
		inline typename std::enable_if<std::is_base_of<Message, T1>::value, bool>::type
		Bind(std::string name, ServiceMethodType44<T, T1> func)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod4<T, T1>>(name, component, func));
		}

		template<typename T>
		bool Bind(std::string name, ServiceMethodType6<T> func)
		{
			T * component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod6<T>>(name, component, func));
		}

		template<typename T>
		bool Bind(std::string name, ServiceMethodType7<T> func)
		{
			T* component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod7<T>>(name, component, func));
		}

		template<typename T, typename T1>
		bool BindAddress(std::string name, ServiceMethodType8<T, T1> func)
		{
			T* component = this->mComponent->Cast<T>();
			return this->AddMethod(std::make_shared<ServiceMethod8<T, T1>>(name, component, func));
		}

	public:
		ServiceMethod* GetMethod(const std::string& name);
		bool AddMethod(std::shared_ptr<ServiceMethod> method);
	private:
		Component * mComponent;
		std::unordered_map<std::string, std::shared_ptr<ServiceMethod>> mMethodMap;
	};
}

namespace Tendo
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
            std::shared_ptr<HttpServiceMethod> httpServiceMethod =
                    std::make_shared<CppHttpServiceMethod<T>>(name, component, std::move(func));
			return this->AddMethod(httpServiceMethod);
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
            std::shared_ptr<HttpServiceMethod> httpServiceMethod =
                std::make_shared<JsonHttpServiceMethod1<T>>(name, component, std::move(func));
            return this->AddMethod(httpServiceMethod);
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
            std::shared_ptr<HttpServiceMethod> httpServiceMethod =
                std::make_shared<JsonHttpServiceMethod2<T>>(name, component, std::move(func));
            return this->AddMethod(httpServiceMethod);
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
            std::shared_ptr<HttpServiceMethod> httpServiceMethod =
                std::make_shared<JsonHttpServiceMethod3<T>>(name, component, std::move(func));
            return this->AddMethod(httpServiceMethod);
        }


    public:		
		void Clear() { this->mHttpMethodMap.clear(); }
		HttpServiceMethod* GetMethod(const std::string& name);
        bool AddMethod(std::shared_ptr<HttpServiceMethod> method);
    private:
		Component * mComponent;
		std::unordered_map<std::string, std::shared_ptr<HttpServiceMethod>> mHttpMethodMap;
    };
}



#endif //SERVER_METHODREGISTER_H
