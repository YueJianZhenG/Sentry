//
// Created by 64658 on 2024/10/26.
//

#pragma once
#include <array>
#include "Kcp/Src/ikcp.h"
#include "Rpc/Common/Message.h"
#include "Net/Network/Tcp/Asio.h"
#include "Proto/Message/IProto.h"
#include "Entity/Component/IComponent.h"

using asio_udp = asio::ip::udp;


namespace kcp
{
	constexpr int TIME_OUT = 20; //xx秒
	constexpr int TIME_OUT_MS = TIME_OUT * 1000; //xx毫秒

	constexpr int UPDATE_INTERVAL = 100;

	constexpr int BUFFER_COUNT = 1200;

	constexpr int RESEND = 2; //快速重传的触发条件。此值设为 2 表示接收到 2 个重复 ACK 时立即重传。
	constexpr int REFRESH_INTERVAL = 20; //表示 KCP 执行内部状态更新的间隔时间，数值越小，更新越频繁，也意味着检测重传的频率越高
}

namespace kcp
{
	class IClient
	{
	public:
		virtual void StartReceive() { };
		virtual bool Update(long long t) = 0;
		virtual void Send(const char * buf, int len) = 0;
		virtual void Send(std::unique_ptr<rpc::Message> & message) { };
	};

	inline int OnKcpSend(const char *buf, int len, struct IKCPCB *kcp, void *user)
	{
		IClient * client = (IClient*)user;
		if(client != nullptr)
		{
			client->Send(buf, len);
		}
		return 0;
	}
}
