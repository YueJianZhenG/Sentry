#pragma once
#include<list>
#include<string>
#include<vector>
#include"RedisAny.h"
#include"XCode/XCode.h"
#include"Json/JsonWriter.h"
#include"google/protobuf/message.h"
#include"Async/RpcTaskSource.h"
#include"Message/ProtoMessage.h"
using namespace google::protobuf;

namespace Sentry
{
	enum class RedisRespType
	{
		REDIS_NONE,
		REDIS_STRING,
		REDIS_ERROR,
		REDIS_NUMBER,
		REDIS_ARRAY,
		REDIS_BIN_STRING,

	};
}

namespace RedisCommand
{
    namespace Str
    {
        static const char * Get = "GET";
        static const char * Set = "SET";
        static const char * Append = "APPEND";
        static const char * AddOne = "INCR";
        static const char * SubOne = "DECR";

    }
}

namespace Sentry
{
    class RedisTask;
    class LuaRedisTask;
 	class RedisRequest : public Tcp::ProtoMessage, public std::enable_shared_from_this<RedisRequest>
    {
    public:
        RedisRequest(const std::string & cmd);
		~RedisRequest() { }
    public:
		const std::string ToJson();
		int Serailize(std::ostream &os) final;
		std::shared_ptr<RedisTask> MakeTask(int ms = 0);
        std::shared_ptr<LuaRedisTask> MakeLuaTask(lua_State * lua, int ms = 0);
    public:
		template<typename ... Args>
		static std::shared_ptr<RedisRequest> Make(const std::string & cmd, Args &&... args);

		static std::shared_ptr<RedisRequest> MakeLua(const std::string & key, const std::string & func, const std::string & json);

        template<typename ... Args>
        static void InitParameter(std::shared_ptr<RedisRequest> self, Args &&... args);

    public:
		void AddParameter(int value);
		void AddParameter(long long value);
		void AddParameter(const Message & message);
		void AddParameter(const std::string & value);
		void AddString(const char * str, size_t size);

        long long GetTaskId() const { return this->mTaskId;}
        const std::string & GetCommand() const { return this->mCommand;}
	private:
        static void Encode(std::shared_ptr<RedisRequest> self) {}
        template<typename T, typename... Args>
        static void Encode(std::shared_ptr<RedisRequest> self, const T &t, Args... args)
        {
            self->AddParameter(t);
			RedisRequest::Encode(self, std::forward<Args>(args)...);
        }
    private:
        long long mTaskId;
        std::string mCommand;
        std::list<std::string> mParameters;
    };

	template<typename ... Args>
	std::shared_ptr<RedisRequest> RedisRequest::Make(const std::string& cmd, Args&& ... args)
	{
		std::shared_ptr<RedisRequest> request = std::make_shared<RedisRequest>(cmd);
		RedisRequest::InitParameter(request, std::forward<Args>(args)...);
		return request;
	}

    template<typename ... Args>
    void RedisRequest::InitParameter(std::shared_ptr<RedisRequest> self, Args &&...args)
    {
		RedisRequest::Encode(self, std::forward<Args>(args)...);
    }

    class RedisResponse
    {
	public:
		RedisResponse();
		~RedisResponse();
    public:
        bool IsOk();
		void Clear();
    private:
        int OnDecodeArray(std::istream & os);
        int OnReceiveFirstLine(std::istream & os);
    public:
        int OnRecvLine(std::istream & os);
        int OnRecvMessage(std::istream & os);
    public:
		bool HasError();
		template<typename T>
        const T * Cast(size_t index);
		const RedisAny * Get(size_t index);
        size_t GetArraySize() { return this->mArray.size();}
        RedisRespType GetType() const { return this->mType; }
        long long GetNumber() const { return this->mNumber; }
        const std::string & GetString() const { return this->mString;}
        const std::vector<RedisAny *> & GetArray() const { return this->mArray; }
    private:
        int mDataSize;
        int mLineCount;
        int mDataCount;
        long long mNumber;
        std::string mString;
        RedisRespType mType;
        std::vector<RedisAny *> mArray;
    };

    template<typename T>
    const T *RedisResponse::Cast(size_t index)
    {
        const RedisAny *any = this->Get(index);
        return dynamic_cast<const T *>(any);
    }

    class RedisTask : public IRpcTask<RedisResponse>
    {
    public:
        RedisTask(std::shared_ptr<RedisRequest> request, int ms);
        ~RedisTask() = default;
    public:
        long long GetRpcId() final { return this->mTaskId; }
        void OnResponse(std::shared_ptr<RedisResponse> response) final;
        std::shared_ptr<RedisResponse> Await() { return this->mTask.Await(); }
    private:
        long long mTaskId;
		std::shared_ptr<RedisRequest> mRequest;
		TaskSource<std::shared_ptr<RedisResponse>> mTask;
    };

    class LuaRedisTask : public IRpcTask<RedisResponse>
    {
    public:
        LuaRedisTask(lua_State * lua, std::shared_ptr<RedisRequest> request, int ms);
        ~LuaRedisTask();
    public:
        int Await();
        long long GetRpcId() final { return this->mTaskId;}
        void OnResponse(std::shared_ptr<RedisResponse> response) final;
    private:
        int mRef;
        lua_State * mLua;
        long long mTaskId;
		std::shared_ptr<RedisRequest> mRequest;
    };
}

