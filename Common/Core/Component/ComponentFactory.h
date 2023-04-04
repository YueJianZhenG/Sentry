#pragma once
#include<queue>
#include<memory>
#include<unordered_map>
#include"Component.h"
namespace Tendo
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

namespace Tendo
{
	class ComponentFactory
	{
	 public:
		template<typename T>
		static bool Add(const std::string name)
		{
//			if (std::is_base_of<IServiceBase, T>::value)
//			{
//				if (std::is_base_of<ISystemUpdate, T>::value)
//				{
//					throw std::logic_error(name + " is in ISystemUpdate child class");
//				}
//				if (std::is_base_of<IFrameUpdate, T>::value)
//				{
//					throw std::logic_error(name + " is in IFrameUpdate child class");
//				}
//				if (std::is_base_of<ISecondUpdate, T>::value)
//				{
//					throw std::logic_error(name + " is in ISecondUpdate child class");
//				}
//				if (std::is_base_of<ILastFrameUpdate, T>::value)
//				{
//					throw std::logic_error(name + " is in ILastFrameUpdate child class");
//				}
//			}
			auto iter = mTypeInfoMap.find(name);
			if (iter != mTypeInfoMap.end())
			{
				throw std::logic_error("add " + name + " failure");
			}
			auto type = new TypeProxy<T>(name);
			mTypeInfoMap.insert(std::make_pair(type->Name, type));
			mTypeInfoMap1.insert(std::make_pair(typeid(T).hash_code(), type));
			return true;
		}

		template<typename T>
		static Type* GetType()
		{
			size_t hash = typeid(T).hash_code();
			auto iter = mTypeInfoMap1.find(hash);
			return iter != mTypeInfoMap1.end() ? iter->second : nullptr;
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
			return iter != mTypeInfoMap1.end() ? iter->second : nullptr;
		}

		template<typename T>
		static std::unique_ptr<Component> CreateComponent();
		static std::unique_ptr<Component> CreateComponent(const std::string& name);

		static Type* GetType(const std::string& name);
	 private:
		static std::string mEmpty;
		static std::unordered_map<size_t, Type*> mTypeInfoMap1;
		static std::unordered_map<std::string, Type*> mTypeInfoMap;
	};

	template<typename T>
	std::unique_ptr<Component> Tendo::ComponentFactory::CreateComponent()
	{
		size_t key = typeid(T).hash_code();
		auto iter = mTypeInfoMap1.find(key);
		if (iter == mTypeInfoMap1.end())
		{
			return nullptr;
		}
		Type* type = iter->second;
		return CreateComponent(type->Name);		
	}
#define REGISTER_COMPONENT(type) ComponentFactory::Add<type>(#type)
}// namespace Sentry