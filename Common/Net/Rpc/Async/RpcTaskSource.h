#pragma once

#include"Lua/Engine/Define.h"
#include"Rpc/Common/Message.h"
#include"Async/Source/TaskSource.h"
#include"Async/Lua/LuaWaitTaskSource.h"
namespace acs
{
    template<typename T>
    class IRpcTask
    {
    public:
        explicit IRpcTask(int id) :mRpcId(id), mTimerId(0) { }
        virtual ~IRpcTask() = default;
    public:
        inline int GetRpcId() noexcept { return mRpcId; }
		inline void SetTimerId(long long id) { this->mTimerId = id; }
		inline long long GetTimerId() const { return this->mTimerId; }
	public:
		virtual void OnTimeout() = 0;
		virtual void OnResponse(std::unique_ptr<T> & response) noexcept = 0;
    private:
        int mRpcId;
		long long mTimerId;
    };

	class RpcTaskSource final : public WaitTaskSourceBase, public IRpcTask<rpc::Message>
    {
    public:

        explicit RpcTaskSource(int id): IRpcTask<rpc::Message>(id), mMessage(nullptr) { }
    public:
		void OnTimeout() final;
		inline std::unique_ptr<rpc::Message> Await() noexcept;
		inline void OnResponse(std::unique_ptr<rpc::Message>& response) noexcept final;
	private:
		std::unique_ptr<rpc::Message> mMessage;
    };

	inline void RpcTaskSource::OnResponse(std::unique_ptr<rpc::Message>& response) noexcept
	{
		this->mMessage = std::move(response);
		this->ResumeTask();
	}

	inline std::unique_ptr<rpc::Message> RpcTaskSource::Await() noexcept
	{
		this->YieldTask();
		return std::move(this->mMessage);
	}

	class LuaRpcTaskSource  final:  public IRpcTask<rpc::Message>
#ifdef __SHARE_PTR_COUNTER__
	, public memory::Object<LuaWaitTaskSource>
#endif
	{
	 public:
		LuaRpcTaskSource(lua_State * lua, int id);
	 public:
		void OnTimeout() final;
		inline int Await() noexcept { return this->mTask.Await(); }
		void OnResponse(std::unique_ptr<rpc::Message>& response) noexcept final;
	 private:
		LuaWaitTaskSource mTask;
	};
}