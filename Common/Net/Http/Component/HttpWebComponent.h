//
// Created by zmhy0073 on 2022/6/21.
//

#ifndef APP_HTTPWEBCOMPONENT_H
#define APP_HTTPWEBCOMPONENT_H
#include "Http/Client/Http.h"
#include "Event/Base/IEvent.h"
#include "Http/Common/Config.h"
#include "Http/Common/httpHead.h"
#include "Net/Record/RecordInfo.h"
#include "Http/Common/ContentType.h"
#include "Entity/Component/Component.h"
#include "Server/Component/ITcpComponent.h"


namespace http
{
	class Content;
    class Request;
    class Response;
	class Content;
	class Session;
}

namespace http
{
	struct Token
	{
		int UserId = 0;
		int Access = 0; //权限
		long long ExpTime = 0; //过期时间
	};
}

namespace acs
{
	class HttpMethodConfig;
}

namespace help
{
	DEFINE_STATIC_EVENT(HttpRequestEvent, const acs::HttpMethodConfig *, http::Request *, http::Response *);
}

namespace acs
{
	class HttpWebComponent final : public Component, public IServerRecord,
								   public IRpc<http::Request, http::Response>, public ITcpListen
    {
    public:
        HttpWebComponent();
    private:
		bool LateAwake() final;
		void OnRecord(json::w::Document& document) final;
	private:
		void StartClose(int id, int) final;
		void OnClientError(int id, int code) final;
		bool OnListen(tcp::Socket *socket) noexcept final;
		void OnReadHead(http::Request *request, http::Response *response) noexcept final;
		void OnMessage(int id, http::Request * request, http::Response * response) noexcept final;
		void OnApi(const HttpMethodConfig* config, http::Request * request, http::Response * response) noexcept;
		void Invoke(const HttpMethodConfig* config, http::Request * request, http::Response * response) noexcept;
	private:
		bool ReadMessageBody(int id, std::unique_ptr<http::Content> content, int timeout);
		HttpStatus AuthToken(const HttpMethodConfig* config, http::Request *request) noexcept;
		HttpStatus OnNotFound(const std::string & path, std::unique_ptr<http::Content> & content) noexcept;
	private:
		bool SendResponse(int fd, HttpStatus code, int timeout);
		bool SendResponse(int fd, HttpStatus code, std::unique_ptr<http::Content> httpContent, int timeout);
		HttpStatus CreateContent(const HttpMethodConfig* config, const http::Head & head, std::unique_ptr<http::Content> & content) noexcept;
	public:
		bool AddRootDirector(const std::string & dir);
		std::shared_ptr<http::Session> GetClient(int id);
	private:
		std::string mPath;
		http::Config mConfig;
		record::Info mRecordInfo;
		http::ContentFactory mFactory;
		math::NumberPool<int> mNumPool;
		std::vector<std::string> mRoots;
		class CoroutineComponent * mCoroutine;
		std::vector<std::shared_ptr<http::Session>> mObjectPool;
		custom::HashMap<std::string, class HttpService *> mHttpServices;
		std::unordered_map<int, std::shared_ptr<http::Session>> mHttpClients;
	};
}


#endif //APP_HTTPWEBCOMPONENT_H
