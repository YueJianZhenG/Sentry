//
// Created by yy on 2025/10/23.
//

#ifndef LUAEVENTCOMPONENT_H
#define LUAEVENTCOMPONENT_H
#include "Entity/Component/Component.h"

namespace acs
{
	class LuaEventComponent : public Component
	{
	public:
		LuaEventComponent();
		~LuaEventComponent() final = default;
	private:
		bool LateAwake() final;
	private:
		void OnPlayerLogin(long long playerId);
	private:
		class LuaComponent * mLuaProxy;
		class EventProxyComponent * mEventProxy;
	};

}



#endif //LUAEVENTCOMPONENT_H
