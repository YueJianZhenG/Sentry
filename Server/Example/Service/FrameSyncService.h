//
// Created by 64658 on 2025/10/15.
//

#ifndef APP_FRAMESYNCSERVICE_H
#define APP_FRAMESYNCSERVICE_H

#include "Rpc/Service/RpcService.h"
#include "Message/c2s/c2s.pb.h"

namespace acs
{
	class FrameSyncService : public RpcService, public IFrameUpdate
	{
	public:
		FrameSyncService();
	private:
		bool OnInit() final;
	private:
		int Login(const rpc::Message & request);
		int Logout(const rpc::Message & request);
		int Commit(const c2s::frame::request & request);
	private:
		void OnFrameUpdate(int elapse) noexcept final;
	private:
		class PlayerComponent * mPlayerMgr;
	};
}



#endif //APP_FRAMESYNCSERVICE_H
