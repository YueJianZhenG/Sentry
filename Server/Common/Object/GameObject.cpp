#include"GameObject.h"
#include<Define/CommonDef.h>
#include<Core/Applocation.h>
#include<Module/Component.h>
namespace Sentry
{
	GameObject::GameObject(const long long id)
		: mGameObjectId(id)
	{

	}

	GameObject::GameObject(const long long id, const std::string & address)
		: mGameObjectId(id)
	{
		mSessionAddress = address;
	}

	Component * GameObject::AddComponentByName(const std::string name)
	{
		Component * component = this->GetComponentByName(name);
		
		return component;
	}

	void GameObject::GetAllComponent(SayNoArray<Component*>& mConponentArray)
	{
		auto iter = this->mComponentMap.begin();
		for (; iter != this->mComponentMap.end(); iter++)
		{
			Component * component = iter->second;
			mConponentArray.push_back(component);
		}
	}

	void GameObject::OnDestory()
	{
		this->DestoryComponents();
	}

	Component * GameObject::GetComponentByName(const std::string name)
	{
		ComponentIter iter = this->mComponentMap.find(name);
		if (iter != this->mComponentMap.end())
		{
			Component * component = iter->second;
			return component->IsActive() ? component : nullptr;
		}
		return nullptr;
	}

	bool GameObject::RemoveComponentByName(const std::string & name)
	{
		ComponentIter iter = this->mComponentMap.find(name);
		if (iter != this->mComponentMap.end())
		{
			Component * component = iter->second;
			component->SetActive(false);
			return true;
		}
		return false;
	}

	void GameObject::PollComponent(float t)
	{
		while (!this->mWaitStartComponents.empty())
		{
			Component * component = this->mWaitStartComponents.front();
			component->OnFrameStart();
			this->mWaitStartComponents.pop();
		}

		ComponentIter iterator = this->mComponentMap.begin();
		for (; iterator != this->mComponentMap.end();)
		{
			Component * pComponent = iterator->second;
			if (pComponent != nullptr)
			{
				if (pComponent->IsActive())
				{
					this->mComponentMap.erase(iterator++);
					pComponent->OnDestory();
					continue;
				}
				pComponent->OnFrameUpdate(t);		
			}
			iterator++;
		}
	}

	

	bool GameObject::RemoveComponent(const std::string & name)
	{
		ComponentIter iter = this->mComponentMap.find(name);
		if (iter != this->mComponentMap.end())
		{
			Component * component = iter->second;
			component->SetActive(false);
			return true;
		}
		return false;
	}

	void GameObject::DestoryComponents()
	{
		ComponentIter iterator = this->mComponentMap.begin();
		for (; iterator != this->mComponentMap.end(); iterator++)
		{
			Component * component = iterator->second;
			component->OnDestory();
		}
		this->mComponentMap.clear();
		while (!this->mWaitStartComponents.empty())
		{
			Component * component = this->mWaitStartComponents.front();
			component->OnDestory();
			this->mWaitStartComponents.pop();
		}
	}
}





