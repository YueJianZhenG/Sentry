//
// Created by 64658 on 2025/10/18.
//

#ifndef APP_EVENTPROXYCOMPONENT_H
#define APP_EVENTPROXYCOMPONENT_H

#include <memory>
#include <typeindex>
#include "Util/Tools/Guid.h"
#include "Entity/Component/Component.h"


namespace acs
{
	class IEventFactory
	{
	public:
		virtual ~IEventFactory() = default;
	public:
		virtual bool Del(long long id) = 0;
		virtual std::type_index Type() const = 0;
	};

// 具体工厂：管理具有固定签名 (Args...) 的回调集合
	template<typename... Args>
	class EventFactoryT : public IEventFactory
	{
	public:
		using Callback = std::function<void(Args...)>;
		EventFactoryT() = default;
		~EventFactoryT() override = default;


		// 添加回调，返回事件 id（>0）
		inline long long Add(Callback cb)
		{
			long long id = help::ID::Gen();
			callbacks.emplace(id, std::move(cb));
			return id;
		}

		// 删除回调，成功返回 true（并回收 id）
		inline bool Del(long long id) override
		{
			auto it = callbacks.find(id);
			if (it == callbacks.end()) {
				return false;
			}
			callbacks.erase(it);
			return true;
		}

		// 触发所有回调
		inline int Invoke(Args &&... args)
		{
			int count = 0;
			auto iter = this->callbacks.begin();
			for(; iter != this->callbacks.end(); iter++)
			{
				++count;
				iter->second(std::forward<Args>(args)...);
			}
			return count;
		}
		inline std::type_index Type() const override { return typeid(EventFactoryT<Args...>); }
	private:
		std::unordered_map<int, Callback> callbacks;
	};

	class EventProxyComponent final : public Component
	{
	public:
		EventProxyComponent() = default;
		~EventProxyComponent() final = default;

		template<typename T, typename... Args>
		inline long long SubEvent(const std::string &id, T* obj, void (T::*func)(Args...))
		{
			if (!obj || !func) {
				return -1;
			}
			using FactoryType = EventFactoryT<Args...>;
			auto factory = GetOrCreateFactory<FactoryType>(id);
			if (factory == nullptr) {
				return -1;
			}

			typename FactoryType::Callback cb = [obj, func](Args... args) {
				(obj->*func)(args...);
			};
			return factory->Add(std::move(cb));
		}

		// 订阅任意 std::function
		template<typename... Args>
		inline long long SubEventFunc(const std::string &id, std::function<void(Args...)> cb)
		{
			if(!cb) {
				return -1;
			}
			using FactoryType = EventFactoryT<Args...>;
			auto factory = GetOrCreateFactory<FactoryType>(id);
			if(factory == nullptr) {
				return -1;
			}
			return factory->Add(std::move(cb));
		}

		// 触发事件；如果没有对应签名的工厂则什么都不做
		template<typename... Args>
		inline int Trigger(const std::string &id, Args... args)
		{
			using FactoryType = EventFactoryT<Args...>;
			IEventFactory* base = GetFactory(id);
			if (base == nullptr) {
				return 0;
			}
			// 类型必须匹配
			if (base->Type() != typeid(FactoryType)) {
				return 0;
			}
			auto * factory = static_cast<FactoryType*>(base);
			return factory->Invoke(std::forward<Args>(args)...);
		}

		// 删除事件（通过事件 id）。返回是否成功
		// 如果工厂不存在或签名不匹配，返回 false
		template<typename... Args>
		inline bool DelEvent(const std::string &id, long long eventId)
		{
			using FactoryType = EventFactoryT<Args...>;
			IEventFactory* base = GetFactory(id);
			if (!base) {
				return false;
			}
			if (base->Type() != typeid(FactoryType)) {
				return false;
			}
			auto * factory = static_cast<FactoryType*>(base);
			return factory->Del(eventId);
		}

		// 可选：删除整个 id 下的工厂（移除所有订阅）
		inline bool RemoveFactory(const std::string &id)
		{
			auto it = factories.find(id);
			if (it == factories.end()) {
				return false;
			}
			factories.erase(it);
			return true;
		}

	private:
		// 获取工厂（不创建）
		inline IEventFactory* GetFactory(const std::string &id)
		{
			auto it = factories.find(id);
			if (it == factories.end()) {
				return nullptr;
			}
			return it->second.get();
		}

		// 获取或创建工厂（模板化）
		template<typename FactoryType>
		inline FactoryType* GetOrCreateFactory(const std::string &id)
		{
			auto it = factories.find(id);
			if (it != factories.end())
			{
				// 存在则类型必须匹配
				if (it->second->Type() != typeid(FactoryType)) {
					// 签名冲突：已有不同签名的工厂使用同一 id，返回 nullptr
					return nullptr;
				}
				return static_cast<FactoryType*>(it->second.get());
			}
			// 不存在则创建
			auto f = std::make_unique<FactoryType>();
			FactoryType* ptr = f.get();
			factories.emplace(id, std::move(f));
			return ptr;
		}

	private:
		std::unordered_map<std::string, std::unique_ptr<IEventFactory>> factories;
	};

}



#endif //APP_EVENTPROXYCOMPONENT_H
