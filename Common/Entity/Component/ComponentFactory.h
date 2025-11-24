#pragma once
#include<memory>
#include<unordered_map>
#include"Component.h"
namespace acs
{
    template<typename T>
    class TypeProxy : public Type
    {
    public:
        explicit TypeProxy(std::string name) : Type(typeid(T).hash_code(), name) {}

    public:
        std::unique_ptr<Component> New() final
        {
            return std::make_unique<T>();
        }
    };
}

namespace acs
{
	class ComponentFactory
	{
	 public:
		template<typename T>
		static bool Add(const std::string name)
		{
			auto iter = mTypeInfoMap.find(name);
			if (iter != mTypeInfoMap.end())
			{
				return false;
			}
			std::unique_ptr<Type> type = std::make_unique<TypeProxy<T>>(name);
			{
				mTypeInfoMap.emplace(type->Name, type.get());
				mTypeInfoMap1.emplace(typeid(T).hash_code(), std::move(type));
			}
			return true;
		}

		template<typename T>
		static Type* GetType()
		{
			size_t hash = typeid(T).hash_code();
			auto iter = mTypeInfoMap1.find(hash);
			return iter != mTypeInfoMap1.end() ? iter->second.get() : nullptr;
		}
		template<typename T>
		static const std::string & GetName()
		{
			size_t hash = typeid(T).hash_code();
			auto iter = mTypeInfoMap1.find(hash);
			return iter != mTypeInfoMap1.end() ? iter->second->Name : mEmpty;
		}

		static Type* GetType(size_t hash)
		{
			auto iter = mTypeInfoMap1.find(hash);
			return iter != mTypeInfoMap1.end() ? iter->second.get() : nullptr;
		}

		template<typename T>
		static std::unique_ptr<Component> CreateComponent();
		static std::unique_ptr<Component> CreateComponent(const std::string& name);

		static Type* GetType(const std::string& name);
	 private:
		static std::string mEmpty;
		static std::unordered_map<std::string, Type*> mTypeInfoMap;
		static std::unordered_map<size_t, std::unique_ptr<Type>> mTypeInfoMap1;
	};

	template<typename T>
	std::unique_ptr<Component> acs::ComponentFactory::CreateComponent()
	{
		size_t key = typeid(T).hash_code();
		auto iter = mTypeInfoMap1.find(key);
		if (iter == mTypeInfoMap1.end())
		{
			return nullptr;
		}
		return CreateComponent(iter->second->Name);
	}

	template<typename T>
	class ComponentAutoRegister
	{
	public:
		explicit ComponentAutoRegister(const std::string & name) {
			ComponentFactory::Add<T>(name);
		}
	};

#define REGISTER_COMPONENT(type) ComponentFactory::Add<type>(#type)
#define AUTO_REGISTER_COMPONENT(type) static ComponentAutoRegister<type> gRegister_##type(#type)
}// namespace Sentry