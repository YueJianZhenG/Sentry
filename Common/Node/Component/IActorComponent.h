//
// Created by 64658 on 2025/4/3.
//

#ifndef APP_IACTORCOMPONENT_H
#define APP_IACTORCOMPONENT_H
#include "Entity/Actor/Actor.h"
#include "Rpc/Common/Message.h"
namespace acs
{
	class IActorComponent
	{
	public:
		virtual Actor * GetActor(long long id) = 0;
		virtual int Broadcast(std::unique_ptr<rpc::Message> & message) = 0;
	};
}

#endif //APP_IACTORCOMPONENT_H
