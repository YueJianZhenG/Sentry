//
// Created by zmhy0073 on 2022/1/19.
//

#ifndef GAMEKEEPER_HTTPASYNCREQUEST_H
#define GAMEKEEPER_HTTPASYNCREQUEST_H
#include<string>
#include<asio.hpp>
#include<fstream>
#include<unordered_map>
#include"Json/JsonWriter.h"
#include"Json/JsonReader.h"
#include<Network/Http/Http.h>
#include"Script/LuaInclude.h"
#include"Network/Proto/ProtoMessage.h"
#include"Async/RpcTask/RpcTaskSource.h"


namespace Sentry
{
    class HttpAsyncResponse;
    class HttpTask : public IRpcTask<HttpAsyncResponse>
    {
    public:
        HttpTask(int time = 0);
    public:
        long long GetRpcId() { return this->mTaskId;}
        int GetTimeout() final { return this->mTimeout;}
        void OnResponse(std::shared_ptr<HttpAsyncResponse> response);
        std::shared_ptr<HttpAsyncResponse> Await() { return this->mTask.Await();}
    private:
        int mTimeout;
        long long mTaskId;
        TaskSource<std::shared_ptr<HttpAsyncResponse>> mTask;
    };
    typedef std::shared_ptr<IRpcTask<HttpAsyncResponse>> SharedHttpRpcTask;
}

namespace Sentry
{
    class LuaHttpTask : public IRpcTask<HttpAsyncResponse>
    {
    public:
        LuaHttpTask(lua_State * lua, int timeout);
        ~LuaHttpTask();
    public:
        int Await();
        int GetTimeout() final { return this->mTimeout; }
        long long GetRpcId() final { return this->mTaskId; }
        void OnResponse(std::shared_ptr<HttpAsyncResponse> response) final;
    private:
        int mRef;
        int mTimeout;
        lua_State * mLua;
        long long mTaskId;
    };
}

namespace Sentry
{
	class HttpAsyncRequest : public Tcp::ProtoMessage
    {
	public:
		HttpAsyncRequest(const std::string& method);
	public:
		bool AddHead(const std::string & key, int value);
		bool AddHead(const std::string & key, const std::string & value);
    public:
        std::shared_ptr<HttpTask> MakeTask(int timeout);
        std::shared_ptr<LuaHttpTask> MakeLuaTask(lua_State * lua, int timeout);
    public:
		int Serailize(std::ostream &os) final;
        long long GetTaskId() const { return this->mTaskId; }
        const std::string & GetHost() const { return this->mHost;}
        const std::string & GetPort() const { return this->mPort;}
	protected:
		bool ParseUrl(const std::string & url);
		virtual void WriteBody(std::ostream & os) const = 0;
	private:
        std::string mHost;
        std::string mPort;
        std::string mPath;
        long long mTaskId;
		const std::string mMethod;
        std::unordered_map<std::string, std::string> mHeadMap;
    };
}

namespace Sentry
{
	class HttpGetRequest : public HttpAsyncRequest
	{
	public:
		HttpGetRequest();
	public:
		static std::shared_ptr<HttpGetRequest> Create(const std::string & url);
	protected:
		void WriteBody(std::ostream & os) const final;
	};
}

namespace Sentry
{
	class HttpPostRequest : public HttpAsyncRequest
	{
	public:
		HttpPostRequest();
	public:
		void AddBody(const std::string & content);
		void AddBody(const char * data, size_t size);
		static std::shared_ptr<HttpPostRequest> Create(const std::string & url);
	protected:
		void WriteBody(std::ostream & os) const final;
	private:
		std::string mBody;
	};
}

namespace Sentry
{
    enum class HttpDecodeState
    {
        FirstLine,
        HeadLine,
        Content,
        Finish
    };

    class HttpData
    {
    public:
        void Add(const std::string & data);
        void Add(const char * data, size_t size);
        void Add(const std::string & key, const std::string & data);
    public:
        const std::string & GetData() const { return this->mData;}
        bool Get(const std::string & key, std::string & value) const;

    public:
        void Writer(lua_State *lua) const;
        void WriterString(lua_State * lua, const char * name, const std::string & str) const;
    public:
        std::string mPath;
        std::string mData;
        std::string mMethod;
        std::string mAddress;
        std::string mVersion;
        std::unordered_map<std::string, std::string> mHead;
    };

    class IHttpContent
    {
    public:
        virtual const std::string &GetContent() const = 0;

        virtual const HttpData &GetData() const = 0;

        virtual int OnReceiveLine(asio::streambuf &streamBuffer) = 0;

        virtual int OnReceiveSome(asio::streambuf &streamBuffer) = 0;
    };
}

namespace Sentry
{
    class HttpAsyncResponse : public IHttpContent
    {
    public:
		HttpAsyncResponse(std::fstream * fs = nullptr);
		~HttpAsyncResponse();
    public:
        int OnReceiveLine(asio::streambuf &streamBuffer) final;
        int OnReceiveSome(asio::streambuf &streamBuffer) final;
        HttpStatus GetHttpCode() { return (HttpStatus)this->mHttpCode;}
        const std::string & GetContent() const final { return this->mHttpData.mData;}
	public:
        void SetError(const asio::error_code & code);
		bool GetHead(const std::string & key, int & value) const;
        const asio::error_code & GetCode() const { return this->mCode;}
        bool GetHead(const std::string & key, std::string & value) const;
		const HttpData & GetData() const final { return this->mHttpData;}
    private:
        int mHttpCode;
        HttpData mHttpData;
        asio::error_code mCode;
        std::string mHttpError;
        HttpDecodeState mState;
		std::fstream * mFstream;
    };
}

namespace Sentry
{
	class HttpHandlerRequest : public IHttpContent
	{
	 public:
		HttpHandlerRequest(const std::string & address);
	 public:

        int OnReceiveSome(asio::streambuf &streamBuffer) final;
	 public:
		const std::string& GetPath() const { return this->mHttpData.mPath; }
		const HttpData & GetData() const final { return this->mHttpData;}
		const std::string& GetMethod() const { return this->mHttpData.mMethod; }
		const std::string& GetAddress() const { return this->mHttpData.mAddress; }
		const std::string& GetContent() const final { return this->mHttpData.mData; }
    public:
        bool GetHead(const std::string& key, int & value) const;
        bool GetHead(const std::string& key, std::string& value) const;
    private:
        int OnReceiveLine(asio::streambuf &streamBuffer) final;
    private:
		std::string mUrl;
        HttpData mHttpData;
        HttpDecodeState mState;
	};
}

namespace Sentry
{
	class HttpHandlerResponse : public Tcp::ProtoMessage
	{
	 public:
		HttpHandlerResponse();
		~HttpHandlerResponse();
	 public:
		void SetCode(HttpStatus code);
		void WriteFile(std::fstream * fs);
		void WriteString(const std::string & content);
        void WriteString(const char * content, size_t size);
    public:
        bool AddHead(const char * key, int value);
        bool AddHead(const char * key, const std::string & value);
    private:
		int Serailize(std::ostream &os) final;
	 private:
		size_t mCount;
		HttpStatus mCode;
		size_t mContentSize;
		std::string mContent;
		std::fstream * mFstream;
		std::unordered_map<std::string, std::string> mHeadMap;
	};
}
#endif //GAMEKEEPER_HTTPASYNCREQUEST_H
