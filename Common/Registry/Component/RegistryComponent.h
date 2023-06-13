//
// Created by MyPC on 2023/4/15.
//

#ifndef APP_REGISTRYCOMPONENT_H
#define APP_REGISTRYCOMPONENT_H
#include"Entity/Component/Component.h"
namespace Tendo
{
	class RedisTcpClient;
	class RegistryComponent : public Component, public IComplete
	{
	public:
		RegistryComponent() = default;
	private:
		void Complete() final;
		bool LateAwake() final;
		bool RegisterServer() const;
	private:
		class Actor * mThisActor;
		class CoroutineComponent * mCorComponent;
	};
}

#endif //APP_REGISTRYCOMPONENT_H
