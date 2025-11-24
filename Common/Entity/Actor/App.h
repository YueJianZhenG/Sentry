#pragma once
#include"Network/Tcp/Asio.h"
#include"Core/Singleton/Singleton.h"
#include"Server/Config/ServerConfig.h"
#include"Async/Component/CoroutineComponent.h"
#include"Node/Component/NodeComponent.h"
#include"Node/Actor/Node.h"
namespace acs
{
	enum class ServerStatus
	{
		Init,
		Start, //正在启动
		Ready, //启动完成
		Running, // 正在运行
		Closing	 //正在关闭
	};
}

namespace acs
{
	class RpcService;
	class ProtoComponent;
	class App final : public Node, public Singleton<App>
	{
	 public:
		explicit App(int id, const std::string & name);
	 public:
        inline float GetFps() const { return this->mFps; }
		inline int GetDeltaTime() const { return this->mDeltaTime; }
		inline const ServerConfig & Config() const { return mConfig; }
		inline ServerStatus GetStatus() const { return this->mStatus; }
		inline long long StartTime() const { return this->mStartTime; }
		inline void Sleep(int ms = 1000) { this->mCoroutine->Sleep(ms); }
	 public:
		int Run();
		int Run(const std::string & cmd);
		void Stop();
		bool Refresh();
		bool LoadLang() const;
		long long MakeGuid();
		std::string NewUuid();
		inline ServerConfig & GetConfig() { return this->mConfig; }
		unsigned int StartCoroutine(std::function<void()> && func);
		inline Asio::Context & GetContext() { return this->mContext; }
        bool OnDelComponent(Component *component) final { return false; }
		inline size_t GetEventCount() const { return this->mEventCount; }
		inline long long GetStartUseMemory() const { return this->mStartMemory; }
#ifdef __DEBUG__
		inline bool IsMainThread() const { return this->mMainId == std::this_thread::get_id(); }
#endif
	public:
		template<typename T>
		static inline T * Get() { return App::Inst()->GetComponent<T>(); }
		static inline NodeComponent * ActorMgr() { return App::Inst()->mActor; }
		static inline ProtoComponent * GetProto() { return App::Inst()->mProto; }
		static inline CoroutineComponent* Coroutine() { return App::Inst()->mCoroutine; }
    private:
		bool LoadComponent();
		bool InitComponent();
		void StartAllComponent();
	 private:
		float mFps;
		int mGuidIndex;
		int mDeltaTime; //两帧时间差
		size_t mEventCount;
		ServerStatus mStatus;
		Asio::Context mContext;
		ServerConfig mConfig;
		long long mStartMemory;
		long long mLastGuidTime;
		asio::signal_set mSignal;
        const long long mStartTime;
		NodeComponent * mActor;
		ProtoComponent * mProto;
#ifdef __DEBUG__
		std::thread::id mMainId;
#endif
		CoroutineComponent* mCoroutine;
    };
}// namespace Sentry