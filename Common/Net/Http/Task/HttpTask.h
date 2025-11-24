//
// Created by zmhy0073 on 2022/11/2.
//

#ifndef APP_HTTPTASK_H
#define APP_HTTPTASK_H
#include"Http/Common/HttpResponse.h"
#include"Rpc/Async/RpcTaskSource.h"

namespace acs
{
	class HttpRequestTask final : public IRpcTask<http::Response>, protected WaitTaskSourceBase
    {
    public:
		~HttpRequestTask() final = default;
		explicit HttpRequestTask(int id) : IRpcTask<http::Response>(id) { }
    public:
		void OnTimeout() final;
        inline std::unique_ptr<http::Response> Await() noexcept;
		inline void OnResponse(std::unique_ptr<http::Response>& response) noexcept final;
	private:
		std::unique_ptr<http::Response> mData;
    };

	std::unique_ptr<http::Response> HttpRequestTask::Await() noexcept
	{
		this->YieldTask();
		return std::move(this->mData);
	}

	void HttpRequestTask::OnResponse(std::unique_ptr<http::Response>& response) noexcept
	{
		this->mData = std::move(response);
		this->ResumeTask();
	}

	inline void HttpRequestTask::OnTimeout()
	{
		this->mData = std::make_unique<http::Response>(HttpStatus::REQUEST_TIMEOUT);
		this->ResumeTask();
	}

	class HttpCallbackTask final : public IRpcTask<http::Response>
	{
	public:
		explicit HttpCallbackTask(int id, std::function<void(std::unique_ptr<http::Response>&)> & cb)
			: mCallback(cb), IRpcTask<http::Response>(id) { }
	private:
		void OnTimeout() final;
		inline void OnResponse(std::unique_ptr<http::Response>& response) noexcept final;
	private:
		std::function<void(std::unique_ptr<http::Response>&)> mCallback;
	};

	inline void HttpCallbackTask::OnResponse(std::unique_ptr<http::Response>& response) noexcept
	{
		this->mCallback(response);
	}

	inline void HttpCallbackTask::OnTimeout()
	{
		std::unique_ptr<http::Response> response = std::make_unique<http::Response>();
		{
			response->SetCode(HttpStatus::REQUEST_TIMEOUT);
			this->mCallback(response);
		}
	}
}

namespace acs
{
    class HttpRequestClient;
    class LuaHttpRequestTask final : public IRpcTask<http::Response>
    {
    public:
        explicit LuaHttpRequestTask(int id, lua_State * lua);
        ~LuaHttpRequestTask() final;
    public:     
        int Await() noexcept;
		void OnTimeout() final;
        void OnResponse(std::unique_ptr<http::Response>& response) noexcept final;
    private:
        int mRef;
        lua_State * mLua;
    };
}

#endif //APP_HTTPTASK_H
