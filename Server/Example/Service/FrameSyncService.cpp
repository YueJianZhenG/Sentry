//
// Created by 64658 on 2025/10/15.
//

#include "FrameSyncService.h"
#include "Entity/Actor/App.h"
#include "Rpc/Config/ServiceConfig.h"
namespace acs
{
	FrameSyncService::FrameSyncService()
	{

	}

	bool FrameSyncService::OnInit()
	{
		return true;
	}

	int FrameSyncService::Login(const rpc::Message& request)
	{
		json::r::Document document;
		int sockId = request.SockId();
		const std::string & token = request.GetBody();
		return XCode::Ok;
	}

	int FrameSyncService::Logout(const rpc::Message& request)
	{
		return XCode::Ok;
	}

	int FrameSyncService::Commit(const c2s::frame::request& request)
	{
		return XCode::Ok;
	}

	void FrameSyncService::OnFrameUpdate(int elapse) noexcept
	{

	}
}