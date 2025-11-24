//
// Created by leyi on 2023/10/25.
//

#ifndef APP_IEVENT_H
#define APP_IEVENT_H


#include"Event.h"
#define DEFINE_STATIC_EVENT(name, ...) struct name : public help::EventFactory<name, __VA_ARGS__> {}
namespace help
{
	DEFINE_STATIC_EVENT(PlayerLoginEvent, long long);
	DEFINE_STATIC_EVENT(PlayerLogoutEvent, long long);

	DEFINE_STATIC_EVENT(InnerClientErrorEvent, int);

	DEFINE_STATIC_EVENT(PlayerLoginGateEvent, long long, int);
	DEFINE_STATIC_EVENT(PlayerLogoutGateEvent, long long, int);
	DEFINE_STATIC_EVENT(AddNodeEvent, const std::string &, int);
	DEFINE_STATIC_EVENT(DelNodeEvent, int);

	DEFINE_STATIC_EVENT(OnOneSecondEvent, int); //每秒事件
	DEFINE_STATIC_EVENT(OnNewDayEvent, int, int); //新一天事件 days week
	DEFINE_STATIC_EVENT(OnNewHourEvent, int, int); //新一个小时 day hour
	DEFINE_STATIC_EVENT(OnNewMinuteEvent, int, int); //新一分钟 hour minute

	DEFINE_STATIC_EVENT(OnRedisPublishMessageEvent, const std::string &, const std::string &); //redis订阅消息

}





#endif //APP_IEVENT_H
