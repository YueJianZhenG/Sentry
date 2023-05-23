//
// Created by MyPC on 2023/4/15.
//

#ifndef APP_REGISTRYCOMPONENT_H
#define APP_REGISTRYCOMPONENT_H
#include"Entity/Component/Component.h"
namespace Tendo
{
	class RegistryComponent : public Component, public IComplete
	{
	public:
		RegistryComponent() = default;
	public:
		int Query(const std::string & server = "");
	private:
		void Complete() final;
		bool LateAwake() final;
	private:
		std::shared_ptr<class Server> mActor;
	};
}

#endif //APP_REGISTRYCOMPONENT_H
