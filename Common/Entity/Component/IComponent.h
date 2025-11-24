#pragma once
#include<string>
#include<memory>
#include<limits>
#include"Yyjson/Document/Document.h"
namespace Lua
{
    class CCModule;
}

namespace acs
{
	class Component;
	class IStart
	{
	public:
		virtual ~IStart() = default;
		virtual void OnStart() = 0;
	};

	class IComplete
	{
	public:
		virtual ~IComplete() = default;
		virtual void OnComplete() { }; //启动完毕
	};

	class IAppStop
	{
	public:
		virtual ~IAppStop() = default;
		virtual void OnAppStop() = 0;
	};

	class IDestroy
	{
	public:
		virtual ~IDestroy() = default;
		virtual void OnDestroy() = 0;
	};

	//每帧调用
	class IFrameUpdate
	{
	public:
		virtual ~IFrameUpdate() = default;
		virtual void OnFrameUpdate(int elapse) = 0;
	};

	//每次循环调用
	class ISystemUpdate
	{
	public:
		virtual ~ISystemUpdate() = default;
		virtual void OnSystemUpdate(long long nowMs) = 0;
	};

	//每秒调用
	class ISecondUpdate
	{
	public:
		virtual ~ISecondUpdate() = default;
		virtual void OnSecondUpdate(int tick) = 0;
	};

	//下一帧调用
	class ILastFrameUpdate
	{
	public:
		virtual ~ILastFrameUpdate() = default;
		virtual void OnLastFrameUpdate() = 0;
	};

	// 热重载调用
	class IRefresh
	{
	public:
		virtual ~IRefresh() = default;
		virtual bool OnRefresh() = 0;
	};

	template<typename C, typename T1, typename T2>
	class IRequest
	{
	public:
		virtual ~IRequest() = default;
		virtual int OnRequest(const C & c, const T1 & t1) { return 0; }
		virtual void OnRequestDone(const C & c, const T1 & t1, const T2 & t2) { }
	};

	template<typename T1, typename T2>
	class IRpc
	{
	public:
        virtual ~IRpc() = default;
		virtual void StartClose(int id) { };
		virtual void StartClose(int id, int) { };
		virtual void OnConnectOK(int id) { }
		virtual void OnClientError(int id, int code) { };
		virtual void OnSendFailure(int id, T1 * message) { }
		virtual void OnMessage(T1* request, T2* response) { };
		virtual void OnReadHead(T1* request, T2 * response) { }
		virtual void OnMessage(int, T1* request, T2* response) { };
	};

    class IServerRecord
    {
    public:
    	virtual ~IServerRecord() = default;
        virtual void OnRecord(json::w::Document & document) = 0;
    };

	extern std::string GET_FUNC_NAME(const std::string& fullName);
}

namespace math
{
	template<typename T, size_t limit = 10>
	class NumberPool
	{
	public:
		NumberPool() : mIndex(0), mCount(0) { }
		explicit NumberPool(T start) : mIndex(start), mCount(0) { }
	public:
		inline T BuildNumber() noexcept
		{
			++this->mIndex;
			++this->mCount;
			if(this->mIndex >= this->MaxNum)
			{
				this->mIndex = 1;
			}
			return this->mIndex;
		}
		inline unsigned long long CurrentNumber() const { return this->mCount; }
	private:
		T mIndex;
		unsigned long long mCount;
		T MaxNum = std::numeric_limits<T>::max() - limit;
	};
}
