#include"Component.h"
#include<Entity/Entity.h>
namespace Sentry
{
	Component::Component()
		: mEntity(nullptr)
	{
        this->mEntityId = 0;
        this->mType = nullptr;
	}
	
	Component * Component::GetByHash(size_t hash)
	{
		Type * type = ComponentFactory::GetType(hash);
		if (type == nullptr)
		{
			return nullptr;
		}
		return this->mEntity->GetComponentByName(type->Name);
	}

    Component *Component::GetByName(const std::string &name)
    {
        return this->mEntity->GetComponentByName(name);
    }

    void Component::GetComponents(std::vector<Component *> &components)
    {
        this->mEntity->GetComponents(components);
    }

}