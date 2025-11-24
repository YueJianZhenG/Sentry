//
// Created by zmhy0073 on 2022/6/21.
//

#include "HttpWebComponent.h"
#include "Entity/Actor/App.h"
#include "Timer/Timer/ElapsedTimer.h"
#include "Core/System/System.h"
#include "Http/Client/HttpSession.h"
#include "Http/Service/HttpService.h"
#include "Util/File/DirectoryHelper.h"
#include "Util/File/FileHelper.h"
#include "Server/Config/CodeConfig.h"
#include "Util/Tools/TimeHelper.h"
#include "Auth/Jwt/Jwt.h"
namespace acs
{
	HttpWebComponent::HttpWebComponent()
	{
		this->mConfig.index = "index.html";
		REGISTER_JSON_CLASS_FIELD(http::Config, auth);
		REGISTER_JSON_CLASS_FIELD(http::Config, root);
		REGISTER_JSON_CLASS_FIELD(http::Config, index);
		REGISTER_JSON_CLASS_FIELD(http::Config, domain);
		REGISTER_JSON_CLASS_FIELD(http::Config, header);
		REGISTER_JSON_CLASS_FIELD(http::Config, whiteList);
		REGISTER_JSON_CLASS_FIELD(http::Config, send_timeout);
		REGISTER_JSON_CLASS_FIELD(http::Config, read_timeout);


		this->mCoroutine = nullptr;
		this->mFactory.Add<http::FromContent>(http::Header::FORM);
		this->mFactory.Add<http::JsonContent>(http::Header::JSON);

		this->mFactory.Add<http::TextContent>(http::Header::TEXT);
		this->mFactory.Add<http::TextContent>(http::Header::XML);
		this->mFactory.Add<http::TextContent>(http::Header::TEXT);

		this->mFactory.Add<http::FileContent>(http::Header::ZIP);
		this->mFactory.Add<http::FileContent>(http::Header::CSS);
		this->mFactory.Add<http::FileContent>(http::Header::HTML);
		this->mFactory.Add<http::FileContent>(http::Header::JPG);
		this->mFactory.Add<http::FileContent>(http::Header::PNG);
		this->mFactory.Add<http::FileContent>(http::Header::JPEG);

		this->mFactory.Add<http::BinContent>(http::Header::PB);

		this->mFactory.Add<http::FileContent>(http::Header::MulFromData);

		this->mHttpClients.max_load_factor(0.75);
		this->mHttpClients.reserve(200);
	}

	bool HttpWebComponent::LateAwake()
	{
		std::vector<HttpService*> httpServices;
		this->mApp->GetComponents(httpServices);
		for (HttpService* httpService: httpServices)
		{
			const std::string& name = httpService->GetName();
			this->mHttpServices.Add(name, httpService);
		}

		this->mCoroutine = App::Coroutine();
		ServerConfig * config = ServerConfig::Inst();
		LOG_CHECK_RET_FALSE(config->Get("http", this->mConfig));
		if (!this->mConfig.root.empty())
		{
			for(const std::string & rootDir : this->mConfig.root)
			{
				this->AddRootDirector(rootDir);
				if(!this->mConfig.index.empty() && this->mPath.empty())
				{
					const std::string & index = this->mConfig.index;
					std::string path = fmt::format("{}/{}", rootDir, index);
					if(help::fs::FileIsExist(path))
					{
						this->mPath = path;
					}
				}
			}
		}
		return true;
	}

	bool HttpWebComponent::AddRootDirector(const std::string& dir)
	{
		LOG_CHECK_RET_FALSE(help::dir::DirectorIsExist(dir));
		auto iter = std::find(this->mRoots.begin(), this->mRoots.end(), dir);
		if (iter != this->mRoots.end())
		{
			return false;
		}
		this->mRoots.emplace_back(dir);
		return true;
	}

	void HttpWebComponent::OnReadHead(http::Request* request, http::Response* response) noexcept
	{
		this->mRecordInfo.sum++;
		int sockId = request->GetSockId();
		HttpStatus httpStatus = HttpStatus::OK;
		auto iter = this->mConfig.header.begin();
		std::unique_ptr<http::Content> httpContent;
		for (; iter != this->mConfig.header.end(); iter++)
		{
			const std::string& key = iter->first;
			response->Header().Add(key, iter->second);
		}
		do
		{
			if (request->ConstHeader().Has("upgrade"))
			{
				httpStatus = HttpStatus::UPGRADE_REQUIRED;
				break;
			}
			const std::string& path = request->GetUrl().Path();
			const HttpMethodConfig* httpConfig = HttpConfig::Inst()->GetMethodConfig(path);
			if (httpConfig == nullptr)
			{
				httpStatus = this->OnNotFound(path, httpContent);
				break;
			}

			if (!httpConfig->open)
			{
				httpStatus = HttpStatus::NOT_FOUND;
				break;
			}
			if (!request->IsMethod(httpConfig->type))
			{
				if (request->IsMethod("OPTIONS"))
				{
					response->Header().Add("Access-Control-Allow-Methods", httpConfig->type);
					response->Header().Add("Access-Control-Allow-Headers", httpConfig->headers);
					break;
				}
				httpStatus = HttpStatus::METHOD_NOT_ALLOWED;
				break;
			}

			if (this->mConfig.auth && httpConfig->auth)
			{
				if (this->AuthToken(httpConfig, request) != HttpStatus::OK)
				{
					httpStatus = HttpStatus::UNAUTHORIZED;
					break;
				}
			}

			if (!httpConfig->WhiteList.empty()) //白名单判断
			{
				std::string ip;
				request->GetIp(ip);
				if (httpConfig->WhiteList.find(ip) == httpConfig->WhiteList.end())
				{
					httpStatus = HttpStatus::NOT_FOUND;
					break;
				}
			}
			if (request->IsMethod("GET"))
			{
				this->OnApi(httpConfig, request, response);
				return;
			}
			httpStatus = this->CreateContent(httpConfig, request->ConstHeader(), httpContent);
			if (httpStatus == HttpStatus::OK)
			{
				this->ReadMessageBody(sockId, std::move(httpContent), this->mConfig.read_timeout);
				return;
			}
		}
		while (false);
		if(httpContent == nullptr)
		{
			this->SendResponse(sockId, httpStatus, this->mConfig.send_timeout);
			return;
		}
		this->SendResponse(sockId, httpStatus, std::move(httpContent), this->mConfig.send_timeout);
	}

	HttpStatus HttpWebComponent::CreateContent(const acs::HttpMethodConfig* httpConfig,
			const http::Head& head, std::unique_ptr<http::Content> & content) noexcept
	{
		std::string cont_type;
		if (!head.GetContentType(cont_type))
		{
			return HttpStatus::BAD_REQUEST;
		}
		if (!httpConfig->content.empty())
		{
			if (httpConfig->content.find(cont_type) == std::string::npos)
			{
				return HttpStatus::BAD_REQUEST;
			}
		}
		if (!this->mFactory.New(cont_type, content))
		{
			if(!httpConfig->content.empty())
			{
				return HttpStatus::UNSUPPORTED_MEDIA_TYPE;
			}
			content = std::make_unique<http::TextContent>();
		}
		long long contentLength = 0;
		if (!head.GetContentLength(contentLength))
		{
			return HttpStatus::LENGTH_REQUIRED;
		}
		if (httpConfig->limit > 0 && contentLength >= httpConfig->limit)
		{
			return HttpStatus::BAD_REQUEST;
		}
		if (content->GetContentType() == http::ContentType::MULTIPAR)
		{
			if (httpConfig->upload.empty())
			{
				return HttpStatus::BAD_REQUEST;
			}
			const int limit = httpConfig->limit;
			const std::string& path = httpConfig->upload;
			content->Cast<http::MultipartFromContent>()->Init(path, limit);
		}
		else if (content->GetContentType() == http::ContentType::FILE)
		{
			if (httpConfig->upload.empty())
			{
				return HttpStatus::BAD_REQUEST;
			}
			size_t pos = cont_type.find('/');
			if (pos == std::string::npos)
			{
				return HttpStatus::INTERNAL_SERVER_ERROR;
			}
			long long guid = this->mApp->MakeGuid();
			std::string type = cont_type.substr(pos + 1);
			const std::string& upload = httpConfig->upload;
			const std::string path = fmt::format("{}/{}.{}", upload, guid, type);
			if (!content->Cast<http::FileContent>()->MakeFile(path))
			{
				return HttpStatus::INTERNAL_SERVER_ERROR;
			}
		}
		return HttpStatus::OK;
	}

	HttpStatus HttpWebComponent::OnNotFound(const std::string & path, std::unique_ptr<http::Content> & content) noexcept
	{
		std::string filePath;
		do
		{
			if (path == "/" && !this->mPath.empty())
			{
				filePath = this->mPath;
				break;
			}
			for (const std::string& dir: this->mRoots)
			{
				filePath = fmt::format("{}/{}", dir, path);
				if (!help::fs::FileIsExist(filePath))
				{
					filePath.clear();
					continue;
				}
				break;
			}
		}
		while(false);
		if(filePath.empty())
		{
			return HttpStatus::NOT_FOUND;
		}
		std::unique_ptr<http::FileContent> fileContent = std::make_unique<http::FileContent>();
		{
			std::string contentType = http::Header::Bin;
			size_t pos = filePath.find_last_of('.');
			if (pos != std::string::npos)
			{
				std::string t = filePath.substr(pos + 1);
				contentType = http::GetContentType(t);
			}
			if(!fileContent->OpenFile(filePath, contentType))
			{
				return HttpStatus::NOT_FOUND;
			}
			content = std::move(fileContent);
		}
		return HttpStatus::OK;
	}

	void HttpWebComponent::OnMessage(int id, http::Request* request, http::Response* response) noexcept
	{
		request->SetSockId(id);
		const std::string& path = request->GetUrl().Path();
		const HttpMethodConfig* httpConfig = HttpConfig::Inst()->GetMethodConfig(path);
		if (httpConfig != nullptr)
		{
			this->OnApi(httpConfig, request, response);
			return;
		}
		std::string ip;
		request->Header().Get(http::Header::RealIp, ip);
		LOG_ERROR("[{}:{}] {}", ip, request->GetUrl().Method(), request->ToString());
		this->SendResponse(id, HttpStatus::NOT_FOUND, this->mConfig.send_timeout);
	}

	void HttpWebComponent::OnApi(const acs::HttpMethodConfig* httpConfig,
			http::Request* request, http::Response* response) noexcept
	{
		if (!httpConfig->async)
		{
			this->Invoke(httpConfig, request, response);
			return;
		}
		this->mCoroutine->Start(&HttpWebComponent::Invoke, this, httpConfig, request, response);
	}

	HttpStatus HttpWebComponent::AuthToken(const HttpMethodConfig* config, http::Request* request) noexcept
	{
		std::string token;
		http::Head& head = request->Header();
		const static std::string AUTH("authorization");
		if (!head.Get(AUTH, token))
		{
			return HttpStatus::UNAUTHORIZED;
		}
		if (!config->token.empty() && token == config->token)
		{
			return HttpStatus::OK;
		}
#ifdef __ENABLE_OPEN_SSL__
		json::r::Document document;
		const ServerConfig & conf = this->mApp->Config();
		if(!jwt::Decode(token, conf.GetSecretKey(), document))
		{
			return HttpStatus::UNAUTHORIZED;
		}

		http::Token httpToken;
		document.Get("a", httpToken.Access);
		document.Get("u", httpToken.UserId);
		document.Get("t", httpToken.ExpTime);
		if (httpToken.Access < config->access)
		{
			LOG_ERROR("[{}] user_id:{} permission:{}/{}", config->path,
					httpToken.UserId, httpToken.Access, config->access);
			return HttpStatus::NOT_FOUND;
		}
		long long nowTime = help::Time::NowSec();
		if (httpToken.ExpTime > 0 && nowTime >= httpToken.ExpTime)
		{
			LOG_WARN("[{}:{}] token exp time", token, httpToken.UserId)
			return HttpStatus::UNAUTHORIZED;
		}
		http::FromContent& query = const_cast<http::FromContent&>(request->GetUrl().GetQuery());
		{
			query.Set(http::query::UserId, httpToken.UserId);
			query.Set(http::query::Access, httpToken.Access);
		}
#endif
		return HttpStatus::OK;
	}

	std::shared_ptr<http::Session> HttpWebComponent::GetClient(int id)
	{
		auto iter = this->mHttpClients.find(id);
		return iter == this->mHttpClients.end() ? nullptr : iter->second;
	}


	void HttpWebComponent::Invoke(const HttpMethodConfig* config, http::Request* request, http::Response* response) noexcept
	{
		this->mRecordInfo.wait++;
		int sockId = request->GetSockId();
		response->SetCode(HttpStatus::OK);
		HttpStatus httpStatus = HttpStatus::OK;
		std::shared_ptr<http::Session> session = this->GetClient(sockId); //保留一次引用计数，避免回调执行回来了,对象没了
		do
		{
			http::Head& head = request->Header();
			if (this->mConfig.auth && config->auth)
			{
				httpStatus = this->AuthToken(config, request);
				if (httpStatus != HttpStatus::OK)
				{
					break;
				}
			}
			HttpService* httpService = this->mHttpServices.Find(config->service);
			if (httpService == nullptr)
			{
				std::string ip;
				httpStatus = HttpStatus::NOT_FOUND;
				head.Get(http::Header::RealIp, ip);
				LOG_ERROR("[{}] {} {}", ip, request->GetUrl().Path(), HttpStatusToString(HttpStatus::NOT_FOUND));
				break;
			}
#ifdef __DEBUG__
			timer::ElapsedTimer timer;
#endif
			int code = XCode::Ok;
			try
			{
				code = httpService->Invoke(config, *request, *response);
			}
			catch(const std::exception & e)
			{
				code = XCode::ThrowError;
				LOG_FATAL("call http [{}.{}] => {}", config->service, config->method, e.what());
			}
			catch(...)
			{
				code = XCode::ThrowError;
				LOG_FATAL("call http [{}.{}] => unknown exception error", config->service, config->method);
			}
			if (response->GetBody() == nullptr)
			{
				json::w::Document document;
				const std::string & desc = CodeConfig::Inst()->GetDesc(code);
				{
					document.Add("code", code);
					document.Add("error", desc);
				}
				response->SetContent(document);
			}
#ifdef __DEBUG__
			if (code != XCode::Ok)
			{
				std::string ip;
				head.Get(http::Header::RealIp, ip);
				const http::Url& url = request->GetUrl();
				const std::string & desc = CodeConfig::Inst()->GetDesc(code);
				LOG_WARN("[{}ms] ({})[{}] code:{} => {}", timer.GetMs(), url.Method(), url.ToStr(), code, desc);
			}
#endif
			httpStatus = response->Code();
		}
		while (false);
		this->mRecordInfo.wait--;
		help::HttpRequestEvent::Trigger(config, request, response);
		this->SendResponse(sockId, httpStatus, this->mConfig.send_timeout);
	}

	void HttpWebComponent::OnRecord(json::w::Document& document)
	{
		size_t byteCount = 0;
		for(auto iter = this->mHttpClients.begin(); iter != this->mHttpClients.end(); iter++)
		{
			byteCount += sizeof(http::Session);
			byteCount += iter->second->SendBufferBytes();
			byteCount += iter->second->RecvBufferBytes();
		}

		std::unique_ptr<json::w::Value> data = document.AddObject("web");
		{
			data->Add("sum", this->mRecordInfo.sum);
			data->Add("conn", this->mRecordInfo.conn);
			data->Add("wait", this->mRecordInfo.wait);
			data->Add("client", this->mHttpClients.size());
			data->Add("success", this->mRecordInfo.success);
			data->Add("failure", this->mRecordInfo.failure);
			data->Add("timeout", this->mRecordInfo.timeout);

			data->Add("memory", byteCount);
		}
	}

	void HttpWebComponent::StartClose(int id, int code)
	{
		auto iter = this->mHttpClients.find(id);
		if(iter != this->mHttpClients.end())
		{
			iter->second->StartClose(code);
			this->mHttpClients.erase(iter);
		}
	}

	void HttpWebComponent::OnClientError(int id, int code)
	{
		if(code == XCode::NetTimeout)
		{
			this->mRecordInfo.timeout++;
		}
		auto iter = this->mHttpClients.find(id);
		if(iter != this->mHttpClients.end())
		{
			if(code == XCode::Ok && this->mObjectPool.size() < this->mConfig.pool)
			{
				this->mObjectPool.emplace_back(iter->second); //正常完整的对象缓存
			}
			this->mHttpClients.erase(iter);
		}
		code == XCode::Ok
		? this->mRecordInfo.success++ : this->mRecordInfo.failure++;
	}

	bool HttpWebComponent::OnListen(tcp::Socket* socket) noexcept
	{
		this->mRecordInfo.conn++;
		int sockId = this->mNumPool.BuildNumber();
		std::shared_ptr<http::Session> handlerClient;
		{
			Asio::Context& io = this->mApp->GetContext();
			handlerClient = std::make_shared<http::Session>(this, io);
		}
		handlerClient->StartReceive(sockId, socket);
		this->mHttpClients.emplace(sockId, handlerClient);
		//CONSOLE_LOG_DEBUG("http client count => {}", this->mHttpClients.size());
		return true;
	}

	bool HttpWebComponent::SendResponse(int id, HttpStatus code, int timeout)
	{
		std::shared_ptr<http::Session> httpClient = this->GetClient(id);
		if(httpClient == nullptr)
		{
			this->mRecordInfo.failure++;
			return false;
		}
		return httpClient->StartWriter(code, timeout);
	}

	bool HttpWebComponent::SendResponse(int id, HttpStatus code, std::unique_ptr<http::Content> httpContent, int timeout)
	{
		std::shared_ptr<http::Session> httpClient = this->GetClient(id);
		if(httpClient == nullptr)
		{
			this->mRecordInfo.failure++;
			return false;
		}
		return httpClient->StartWriter(code, std::move(httpContent), timeout);
	}

	bool HttpWebComponent::ReadMessageBody(int id, std::unique_ptr<http::Content> httpContent, int timeout)
	{
		auto iter = this->mHttpClients.find(id);
		if(iter == this->mHttpClients.end())
		{
			return false;
		}
		iter->second->StartReceiveBody(std::move(httpContent), timeout);
		return true;
	}
}