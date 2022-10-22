#include"Unit.h"
#include"App/App.h"
#include"Component/Component.h"
namespace Sentry
{
	Unit::Unit(long long id) : mUnitId(id)
	{
	}

	bool Unit::AddComponent(const std::string& name)
	{
		auto iter = this->mComponentMap.find(name);
		if (iter != this->mComponentMap.end())
		{
			return false;
		}
		return this->AddComponent(name, ComponentFactory::CreateComponent(name));
	}

	bool Unit::AddComponent(const std::string& name, std::unique_ptr<Component> component)
	{
		if (component == nullptr)
		{
			return false;
		}
		if (this->mComponentMap.find(name) != this->mComponentMap.end())
		{
			LOG_ERROR("add " << name << " failure");
			return false;
		}
		component->mName = name;
        component->mUnit = this;
        component->mEntityId = mUnitId;

		if(!component->Awake())
        {
            return false;
        }
        this->mSortComponents.emplace_back(name);
        this->OnAddComponent(component.get());
		this->mComponentMap.emplace(name, std::move(component));
		return true;
	}


	size_t Unit::GetComponents(std::vector<Component*>& components) const
	{
        for(const std::string & name : this->mSortComponents)
        {
            Component * component = this->GetComponentByName(name);
            if(component != nullptr)
            {
                components.emplace_back(component);
            }
        }
        return components.size();
	}

	size_t Unit::GetComponents(std::vector<std::string>& components) const
	{
		components.clear();
		for (const std::string& name : this->mSortComponents)
		{
			Component* component = this->GetComponent<Component>(name);
			if (component != nullptr)
			{
				components.emplace_back(component->GetName());
			}
		}
        return components.size();
	}

	Component* Unit::GetComponentByName(const std::string& name) const
	{
		auto iter = this->mComponentMap.find(name);
		return iter != this->mComponentMap.end() ? iter->second.get() : nullptr;
	}

	bool Unit::RemoveComponent(const std::string& name)
	{
		auto iter = this->mComponentMap.find(name);
		if (iter != this->mComponentMap.end())
		{
			Component * component = iter->second.get();
            if(!this->OnDelComponent(component))
            {
                return false;
            }
			this->mComponentMap.erase(iter);
			return true;
		}
		return false;
	}
}// namespace Sentry
