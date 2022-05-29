//
// Created by zmhy0073 on 2021/10/14.
//

#ifndef GameKeeper_MESSAGESTREAM_H
#define GameKeeper_MESSAGESTREAM_H
#include <Define/CommonLogDef.h>
#include <Define/CommonTypeDef.h>
#include <Protocol/com.pb.h>
#include <Protocol/c2s.pb.h>
#include <queue>

enum class MESSAGE_TYPE
{
	MSG_RPC_REQUEST = 0x01, //服务器请求
	MSG_RPC_RESPONSE = 0x02, //服务器返回
	MSG_RPC_CALL_CLIENT = 0x03, //服务器调用客户端
	MSG_RPC_CLIENT_REQUEST = 0x04, //客户端请求
	MSG_NET_EVENT = 0x05 //redis事件
};

#endif //GameKeeper_MESSAGESTREAM_H
