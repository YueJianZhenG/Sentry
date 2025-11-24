//
// Created by leyi on 2023/6/6.
//

#ifndef APP_EVENT_H
#define APP_EVENT_H
#include<functional>
#include<unordered_map>
#include "Util/Tools/Guid.h"
namespace help
{
	template<typename T, typename ... Args>
	class EventFactory
	{
	public:
		template<typename T1>
		static long long Add(T1 * obj, void (T1::*func)(Args ...)) {
			long long id = help::ID::Gen();
			std::function<void(Args ...)> callback = [obj, func](Args ... args) {
				(obj->*func)(args...);
			};
			mEvents.emplace(id, callback);
			return id;
		}

		static long long Add(std::function<void(Args ...)> && callback) {
			long long id = help::ID::Gen();
			mEvents.emplace(id, callback);
			return id;
		}

		static bool Remove(long long id)
		{
			auto iter = mEvents.find(id);
			if(iter == mEvents.end())
			{
				return false;
			}
			mEvents.erase(iter);
			return true;
		}

		static int Trigger(Args &... args)
		{
			int count = 0;
			auto iter = mEvents.begin();
			for(; iter != mEvents.end(); iter++)
			{
				count++;
				iter->second(std::forward<Args>(args)...);
			}
			return count;
		}
	private:
		static std::unordered_map<long long, std::function<void(Args ...)>> mEvents;
	};

	template<typename T, typename ... Args>
	std::unordered_map<long long, std::function<void(Args ...)>> EventFactory<T, Args...>::mEvents;



}


#endif //APP_EVENT_H
