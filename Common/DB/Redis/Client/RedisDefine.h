#pragma once
#include <list>
#include <string>
#include <utility>
#include <vector>
#include "Lua/Engine/Define.h"
#include "Rpc/Async/RpcTaskSource.h"
#include "Proto/Message/IProto.h"

#ifdef __SHARE_PTR_COUNTER__
#include "Core/Memory/MemoryObject.h"
#endif
namespace redis
{
	namespace type
	{
		constexpr char String = '+';
		constexpr char Error = '-';
		constexpr char Number = ':';
		constexpr char Array = '*';
		constexpr char BinString = '$';
	}

	struct Parameter
	{
	public:
		char type;
		std::string value;
	};
}

namespace redis
{
	class Request final : public tcp::IProto
#ifdef __SHARE_PTR_COUNTER__
			, public memory::Object<Request>
#endif
	{
	public:
		Request() : mTaskId(0){ }
		explicit Request(std::string cmd) : mTaskId(0), mCommand(std::move(cmd)) { }
	public:
		void Clear() final;
		std::string ToString() final;
		int OnSendMessage(std::ostream& os) final;
	public:
		template<typename ... Args>
		static std::unique_ptr<Request> Make(const std::string& cmd, Args&& ... args);

		static std::unique_ptr<Request>
		MakeLua(const std::string& key, const std::string& func, const std::string& json);

		template<typename ... Args>
		static void InitParameter(Request* self, Args&& ... args);
	public:
		inline void SetRpcId(int id) {this->mTaskId = id; }
		inline int GetRpcId() const { return this->mTaskId; }
		inline const std::string& GetCommand() const{ return this->mCommand; }
		inline void SetCommand(const std::string& cmd) { this->mCommand = cmd;}
		inline void SetCommand(const char * cmd, size_t size) { this->mCommand.assign(cmd, size);}
		inline void AddParameter(const std::string& value) { this->mParameters.emplace_back(value); }
		inline void AddParameter(const char* str, size_t size) { this->mParameters.emplace_back(str, size); }
		inline void AddParameter(const json::w::Document & document) {
			wrap::string<true> json;
			if(document.Serialize(json))
			{
				this->mParameters.emplace_back(json.c_str(), json.size());
			}
		}

		template<typename T>
		inline std::enable_if_t<std::is_integral<T>::value || std::is_floating_point<T>::value, void> AddParameter(T value) {
			this->mParameters.emplace_back(std::to_string(value));
		}

		template<typename T>
		inline std::enable_if_t<std::is_integral<T>::value || std::is_floating_point<T>::value, void> AddParameter(const std::vector<T> & value)
		{
			for(const T & item : value) {
				this->mParameters.emplace_back(std::to_string(item));
			}
		}
		inline void AddParameter(const std::vector<std::string > & value) {
			this->mParameters.insert(this->mParameters.end(), value.begin(), value.end());
		}
	private:
		static void Encode(Request* self) { }
		template<typename T, typename... Args>
		static void Encode(Request* self, const T& t, Args... args)
		{
			self->AddParameter(t);
			Request::Encode(self, std::forward<Args>(args)...);
		}

	private:
		int mTaskId;
		std::string mCommand;
		std::list<std::string> mParameters;
	};

	template<typename ... Args>
	std::unique_ptr<Request> Request::Make(const std::string& cmd, Args&& ... args)
	{
		std::unique_ptr<Request> request = std::make_unique<Request>();
		{
			request->SetCommand(cmd);
			Request::InitParameter(request.get(), std::forward<Args>(args)...);
		}
		return request;
	}

	template<typename ... Args>
	void Request::InitParameter(Request* self, Args&& ...args)
	{
		Request::Encode(self, std::forward<Args>(args)...);
	}


	struct Element
	{
		char type = 0;
		std::string message;
		long long number = 0;
		std::list<Element> list;
	public:
		inline bool IsString() const { return this->type == redis::type::String || this->type == redis::type::BinString; }
	};


	class Response final : public ILuaWrite
#ifdef __SHARE_PTR_COUNTER__
			, public memory::Object<Response>
#endif
	{
	public:
		Response();
		~Response()  { this->Clear(); }

	public:
		void Clear();
		int WriteToLua(lua_State* lua) const final;
		int WriteElementLua(const Element & element, lua_State* lua) const;
		inline bool IsOk() const { return this->element.message == "OK"; }
		inline bool HasError() const { return this->element.type == redis::type::Error || this->element.type == 0; }
	public:
		std::string ToString();
	public:
		Element element;
	};
}

namespace acs
{
	class RedisTask final : public acs::IRpcTask<redis::Response>, protected WaitTaskSourceBase
    {
    public:
        explicit RedisTask(int id);
    public:
		inline void OnTimeout() final;
		inline std::unique_ptr<redis::Response> Await() noexcept;
		inline void OnResponse(std::unique_ptr<redis::Response>& response) noexcept final;
    private:
		std::unique_ptr<redis::Response> mMessage;
    };

	inline std::unique_ptr<redis::Response> RedisTask::Await() noexcept
	{
		this->YieldTask();
		return std::move(this->mMessage);
	}

	inline void RedisTask::OnResponse(std::unique_ptr<redis::Response>& response) noexcept
	{
		this->mMessage = std::move(response);
		this->ResumeTask();
	}

	void RedisTask::OnTimeout()
	{
		this->mMessage = std::make_unique<redis::Response>();
		{
			this->mMessage->element.message = "time out";
			this->mMessage->element.type = redis::type::Error;
		}
		this->ResumeTask();
	}

    class LuaRedisTask : public acs::IRpcTask<redis::Response>
    {
    public:
        LuaRedisTask(lua_State * lua, int id);
        ~LuaRedisTask() final;
    public:
        int Await() noexcept;
		void OnTimeout() final;
        void OnResponse(std::unique_ptr<redis::Response>& response) noexcept final;
    private:
        int mRef;
        lua_State * mLua;
    };
}

