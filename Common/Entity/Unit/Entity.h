#pragma once
#include<vector>
#include"Entity/Component/ComponentFactory.h"
namespace acs
{
    class Entity
	{
	 public:
		explicit Entity(long long id);
		Entity(const Entity &) = delete;
		Entity(const Entity &&) = delete;
    	virtual ~Entity() = default;
		Entity & operator = (const Entity &) = delete;
	 public:
		template<typename T>
		inline bool AddComponent();
		bool HasComponent(const std::string& name);
		bool AddComponent(const std::string& name);
		bool AddComponent(const std::string& name, std::unique_ptr<Component> component);

		template<typename T>
		inline T* GetComponent() const;

		template<typename T>
		inline T * GetChildComponent() const;

		template<typename T>
		inline T* GetComponent(const std::string& name) const;

		Component* GetComponentByName(const std::string& name) const;

		template<typename T>
		inline bool RemoveComponent();

		bool RemoveComponent(const std::string& name);

		template<typename T>
		inline T* GetOrAddComponent();

		template<typename T>
		inline T * Cast() { return dynamic_cast<T*>(this); }

	 public:
		virtual bool LateAwake() = 0;
		size_t GetComponents(std::vector<Component*>& components) const;
		size_t GetComponents(std::vector<std::string>& components) const;

        template<typename T>
        size_t GetComponents(std::vector<T *> & components) const;
	 protected:
		virtual void OnAddComponent(Component * component) {}
		virtual bool OnDelComponent(Component * component) { return true; }
		inline void ClearComponents() { this->mComponentMap.clear(); this->mSortComponents.clear(); }
	 public:
		inline long long GetId() const { return this->mId; }
	 private:
		const long long mId;
		std::vector<std::string> mSortComponents;
		std::unordered_map<std::string, std::unique_ptr<Component>> mComponentMap;
	};

	template<typename T>
	T* Entity::GetChildComponent() const
	{
		auto iter = this->mComponentMap.begin();
		for(; iter != this->mComponentMap.end(); iter++)
		{
			T * component = iter->second->Cast<T>();
			if(component != nullptr)
			{
				return component;
			}
		}
		return nullptr;
	}

    template<typename T>
    size_t Entity::GetComponents(std::vector<T *> &components) const
    {
        for(const std::string & name : this->mSortComponents)
        {
            T * component = this->GetComponent<T>(name);
            if(component != nullptr)
            {
                components.emplace_back(component);
            }
        }
        return components.size();
    }

	template<typename T>
	inline T* Entity::GetComponent() const
	{
		Type* type = ComponentFactory::GetType<T>();
		if (type == nullptr)
		{
			auto iter = this->mComponentMap.begin();
			for(; iter != this->mComponentMap.end(); iter++)
			{
				T * component = iter->second->Cast<T>();
				if(component != nullptr)
				{
					return component;
				}
			}
			return nullptr;
		}
		return this->GetComponent<T>(type->Name);
	}

	template<typename T>
	T* Entity::GetComponent(const std::string& name) const
	{
		auto iter = this->mComponentMap.find(name);
		if (iter != this->mComponentMap.end())
		{
			Component* component = iter->second.get();
			return dynamic_cast<T*>(component);
		}
		return nullptr;
	}

	template<typename T>
	inline bool Entity::AddComponent()
	{
		if (this->GetComponent<T>() == nullptr)
		{			
			std::unique_ptr<Component> component = ComponentFactory::CreateComponent<T>();
			if (component == nullptr)
			{
				//const char * name = typeid(T).name();
				return false;
			}
			const std::string& name = ComponentFactory::GetName<T>();
			return this->AddComponent(name, std::move(component));
		}
		return false;
	}

	template<typename T>
	inline bool Entity::RemoveComponent()
	{
		Type* type = ComponentFactory::GetType<T>();
		if (type == nullptr)
		{
			return false;
		}
		return this->RemoveComponent(type->Name);
	}

	template<typename T>
	inline T* Entity::GetOrAddComponent()
	{
		T* component = this->GetComponent<T>();
		if (component == nullptr)
		{
			if(!this->AddComponent<T>())
            {
                return nullptr;
            }
			return this->GetComponent<T>();
		}
		return component;
	}
}