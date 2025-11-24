#include"RpcTaskSource.h"
#include"XCode/XCode.h"
#include"Util/Tools/TimeHelper.h"

namespace acs
{
	void RpcTaskSource::OnTimeout()
	{
		this->mMessage = std::make_unique<rpc::Message>();
		this->mMessage->GetHead().Add(rpc::Header::code, XCode::CallTimeout);
		this->ResumeTask();
	}

	LuaRpcTaskSource::LuaRpcTaskSource(lua_State* lua, int id)
		: IRpcTask<rpc::Message>(id), mTask(lua) { }

	void LuaRpcTaskSource::OnResponse(std::unique_ptr<rpc::Message>& response) noexcept
	{
		int code = response == nullptr
				? XCode::CallTimeout : response->Code();
		this->mTask.SetResult(code, response);
	}

	void LuaRpcTaskSource::OnTimeout()
	{
		std::unique_ptr<rpc::Message> response = std::make_unique<rpc::Message>();
		{
			response->GetHead().Add(rpc::Header::code, XCode::CallTimeout);
		}
		this->mTask.SetResult(XCode::CallTimeout, response);
	}
}
