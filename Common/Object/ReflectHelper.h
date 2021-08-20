#pragma once

#include <Manager/Manager.h>
#include <Service/ServiceBase.h>

namespace Sentry
{
	template<typename Base>
	class ReflectHelper
	{
	public:
		template<typename T>
		static bool Register(const std::string name);

		static Base *Create(const std::string name);

		static bool GetTypeName(const size_t hash, std::string &name);

	private:
		template<typename T>
		static Base *Create();

	private:
		static std::unordered_map<size_t, std::string> mTypeInfoMap;
		static std::unordered_map<std::string, std::function<Base *()>> mCreateActions;
	};

	template<typename Base>
	std::unordered_map<size_t, std::string> ReflectHelper<Base>::mTypeInfoMap;
	template<typename Base>
	std::unordered_map<std::string, std::function<Base *()>> ReflectHelper<Base>::mCreateActions;

	template<typename Base>
	template<typename T>
	inline bool ReflectHelper<Base>::Register(const std::string name)
	{
		size_t hash = typeid(T).hash_code();
		auto iter = mCreateActions.find(name);
		if (iter != mCreateActions.end())
		{
			return false;
		}
		mTypeInfoMap.emplace(hash, name);
		mCreateActions.emplace(name, std::bind(&ReflectHelper::Create<T>));
		return true;
	}

	template<typename Base>
	template<typename T>
	inline Base *ReflectHelper<Base>::Create()
	{
		return new T();
	}

	template<typename Base>
	inline Base *ReflectHelper<Base>::Create(const std::string name)
	{
		auto iter = mCreateActions.find(name);
		if (iter != mCreateActions.end())
		{
			Base *object = iter->second();
			if (object != nullptr)
			{
				Applocation *app = Applocation::Get();
				object->Init(app, name);
				return object;
			}
		}
		return nullptr;
	}

	template<typename Base>
	inline bool ReflectHelper<Base>::GetTypeName(const size_t hash, std::string &name)
	{
		auto iter = mTypeInfoMap.find(hash);
		if (iter != mTypeInfoMap.end())
		{
			name = iter->second;
			return true;
		}
		return false;
	}

#define RegisterManager(ManagerType) ReflectHelper<Manager>::Register<ManagerType>(#ManagerType)
#define RegisterService(ServiceType) ReflectHelper<LocalService>::Register<ServiceType>(#ServiceType)

}// namespace Sentry