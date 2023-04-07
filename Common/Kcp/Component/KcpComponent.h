//
// Created by MyPC on 2023/4/7.
//

#ifndef APP_KCPCOMPONENT_H
#define APP_KCPCOMPONENT_H
#include<thread>
#include<asio.hpp>
#include"Kcp/Common/ikcp.h"
#include"Core/Component/Component.h"
namespace Tendo
{
	class KcpComponent : public Component, public ISecondUpdate
	{
	public:
		bool StartListen(const char * name);
	private:
		bool LateAwake() final;
		void OnSecondUpdate(int tick);
		static int KcpSendCallback(const char * buf, int len, ikcpcb * kcp, void * user);
	 private:
		void Update(asio::io_service & io, long long time);
	 private:
		std::thread * mThread;
	};
}


#endif //APP_KCPCOMPONENT_H
