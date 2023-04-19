//
// Created by MyPC on 2023/4/19.
//

#ifndef APP_EVENTUNIT_H
#define APP_EVENTUNIT_H
#include"Unit.h"


namespace Tendo
{
	class EventUnit : public Unit
	{
	public:
		using Unit::Unit;
		template<typename T>
		int Dispatch(const T * message);
	private:
		std::unordered_map<size_t, std::vector<std::string>> mEvents;
	};

	template<typename T>
	int EventUnit::Dispatch(const T* message)
	{
		int count = 0;
		typedef IEvent<T> EventComponent;
		size_t hash = typeid(T).hash_code();
		auto iter = this->mEvents.find(hash);
		if(iter != this->mEvents.end())
		{
			for(const std::string & name : iter->second)
			{
				Component * component = this->GetComponentByName(name);
				EventComponent * eventComponent = (EventComponent*)component;
				if(eventComponent != nullptr)
				{
					count++;
					eventComponent->Invoke(message);
				}
			}
		}
		else
		{
			std::vector<std::string> tmp;
			this->mEvents.emplace(hash, tmp);
			std::vector<std::string> components;
			this->GetComponents(components);
			for(const std::string & name : components)
			{
				EventComponent * eventComponent = this->GetComponent<EventComponent>(name);
				if(eventComponent != nullptr)
				{
					count++;
					eventComponent->Invoke(message);
					this->mEvents[hash].emplace_back(name);
				}
			}
		}
		return count;
	}
}


#endif //APP_EVENTUNIT_H
