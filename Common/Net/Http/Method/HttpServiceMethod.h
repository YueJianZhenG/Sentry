//
// Created by zmhy0073 on 2021/11/2.
//

#ifndef APP_HTTPSERVICEMETHOD_H
#define APP_HTTPSERVICEMETHOD_H

#include"XCode/XCode.h"
#include"Http/Client/Http.h"
#include"Yyjson/Document/Document.h"
#include"Http/Common/HttpRequest.h"
#include"Http/Common/HttpResponse.h"
namespace acs
{
	template<typename T>
	using HttpMethod = int(T::*)(const http::Request& request, http::Response& response);

	template<typename T>
	using HttpMethod2 = int(T::*)(const http::Request& request, json::w::Document& response);

    template<typename T>
    using HttpJsonMethod1 = int(T::*)(const json::r::Document & request);

    template<typename T>
    using HttpJsonMethod2 = int(T::*)(const json::r::Document & request, json::w::Value & response);

	template<typename T>
	using HttpJsonMethod4 = int(T::*)(const json::r::Document & request, json::w::Document & response);

	template<typename T>
	using HttpJsonMethod5 = int(T::*)(const json::r::Document & request, http::Response & response);

    template<typename T>
    using HttpJsonMethod3 = int(T::*)(json::w::Document & response);


	template<typename T>
	using HttpMethod4 = int(T::*)(http::Response& response);

	template<typename T>
	using HttpFromMethod1= int(T::*)(const http::FromContent & request, json::w::Value & response);

	template<typename T>
	using HttpFromMethod2 = int(T::*)(const http::FromContent & request, http::Response & response);

	template<typename T>
	using HttpFromMethod3 = int(T::*)(const http::FromContent & request, json::w::Document & response);

	template<typename T>
	using HttpFromMethod4 = int(T::*)(const http::Head & request, json::w::Document & response);

	class HttpServiceMethod
	{
    public:
        explicit HttpServiceMethod(std::string  name)  : mName(std::move(name)) { }
	 public:
        const std::string & GetName() const { return this->mName; }
		virtual int Invoke(const http::Request & request, http::Response & response) = 0;

    private:
        const std::string mName;
	};

	template<typename T>
	class CppHttpServiceMethod : public HttpServiceMethod
	{
	 public:
		CppHttpServiceMethod(const std::string & name, T* o, HttpMethod<T> func)
			: HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
		{
		}

	 public:
		int Invoke(const http::Request & request, http::Response & response) final
		{
			return (this->mObj->*mFunction)(request, response);
		}

	 private:
		T* mObj;
		HttpMethod<T> mFunction;
	};

	template<typename T>
	class CppHttpServiceMethod3 : public HttpServiceMethod
	{
	public:
		CppHttpServiceMethod3(const std::string & name, T* o, HttpMethod2<T> func)
				: HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
		{
		}

	public:
		int Invoke(const http::Request & request, http::Response & response) final
		{
			std::unique_ptr<json::w::Document> document2(new json::w::Document());
			int code = (this->mObj->*mFunction)(request, *document2);
			if(code != XCode::Ok)
			{
				return code;
			}
			document2->Add("code", code);
			response.SetContent(*document2);
			return code;
		}

	private:
		T* mObj;
		HttpMethod2<T> mFunction;
	};


	template<typename T>
	class CppHttpServiceMethod2 : public HttpServiceMethod
	{
	public:
		CppHttpServiceMethod2(const std::string & name, T* o, HttpMethod4<T> func)
				: HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
		{
		}

	public:
		int Invoke(const http::Request & request, http::Response & response) final
		{
			return (this->mObj->*mFunction)(response);
		}

	private:
		T* mObj;
		HttpMethod4<T> mFunction;
	};

    template<typename T>
    class JsonHttpServiceMethod1 : public HttpServiceMethod
    {
    public:
        JsonHttpServiceMethod1(const std::string & name, T* o, HttpJsonMethod1<T> func)
            : HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
        {
        }

    public:
        int Invoke(const http::Request& request, http::Response& response) final
		{
			const http::Content* body = request.GetBody();
			if (body == nullptr || body->GetContentType() != http::ContentType::JSON)
			{
				return XCode::CallArgsError;
			}
			const http::JsonContent* jsonData = (const http::JsonContent*)(body);
			std::unique_ptr<json::w::Document> document(new json::w::Document());
			int code = (this->mObj->*mFunction)(jsonData->JsonObject());
			if (code != XCode::Ok)
			{
				return code;
			}
			document->Add("code", code);
			response.SetContent(*document);
			return code;
		}

    private:
        T* mObj;
        HttpJsonMethod1<T> mFunction;
    };

    template<typename T>
    class JsonHttpServiceMethod2 : public HttpServiceMethod
    {
    public:
        JsonHttpServiceMethod2(const std::string & name, T* o, HttpJsonMethod2<T> func)
            : HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
        {
        }

    public:
        int Invoke(const http::Request& request, http::Response& response) final
		{
			const http::Content* body = request.GetBody();
			const http::JsonContent* jsonData = (const http::JsonContent*)(body);

			if (jsonData == nullptr)
			{
				return XCode::CallArgsError;
			}
			if (body->GetContentType() != http::ContentType::JSON)
			{
				return XCode::ParseJsonFailure;
			}
			std::unique_ptr<json::w::Document> document2(new json::w::Document());
			std::unique_ptr<json::w::Value> document3 = document2->AddObject("data");
			int code = (this->mObj->*mFunction)(jsonData->JsonObject(), *document3);
			if (code != XCode::Ok)
			{
				return code;
			}
			document2->Add("code", code);
			response.SetContent(*document2);
			return code;
		}

    private:
        T* mObj;
        HttpJsonMethod2<T> mFunction;
    };

	template<typename T>
	class JsonHttpServiceMethod4 : public HttpServiceMethod
	{
	public:
		JsonHttpServiceMethod4(const std::string & name, T* o, HttpJsonMethod4<T> func)
				: HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
		{
		}

	public:
		int Invoke(const http::Request& request, http::Response& response) final
		{
			const http::Content* body = request.GetBody();
			if (body->GetContentType() != http::ContentType::JSON)
			{
				return XCode::ParseJsonFailure;
			}

			const http::JsonContent* jsonData = (const http::JsonContent*)body;
			std::unique_ptr<json::w::Document> document2(new json::w::Document());
			int code = (this->mObj->*mFunction)(jsonData->JsonObject(), *document2);
			if (code != XCode::Ok)
			{
				return code;
			}
			document2->Add("code", code);
			response.SetContent(*document2);
			return code;
		}

	private:
		T* mObj;
		HttpJsonMethod4<T> mFunction;
	};


	template<typename T>
	class JsonHttpServiceMethod5 : public HttpServiceMethod
	{
	public:
		JsonHttpServiceMethod5(const std::string & name, T* o, HttpJsonMethod5<T> func)
				: HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
		{
		}

	public:
		int Invoke(const http::Request& request, http::Response& response) final
		{
			const http::Content* body = request.GetBody();
			if (body->GetContentType() != http::ContentType::JSON)
			{
				return XCode::CallArgsError;
			}
			const http::JsonContent* jsonData = (const http::JsonContent*)(body);
			return (this->mObj->*mFunction)(jsonData->JsonObject(), response);
		}

	private:
		T* mObj;
		HttpJsonMethod5<T> mFunction;
	};

    template<typename T>
    class JsonHttpServiceMethod3 : public HttpServiceMethod
    {
    public:
        JsonHttpServiceMethod3(const std::string & name, T* o, HttpJsonMethod3<T> func)
            : HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
        {
        }

    public:
        int Invoke(const http::Request & request, http::Response & response) final
		{
			std::unique_ptr<json::w::Document> document2(new json::w::Document());
			int code = (this->mObj->*mFunction)(*document2);
			if (code != XCode::Ok)
			{
				return code;
			}
			document2->Add("code", code);
			response.SetContent(*document2);
			return code;
		}

    private:
        T* mObj;
        HttpJsonMethod3<T> mFunction;
    };


	template<typename T>
	class FromHttpServiceMethod1 : public HttpServiceMethod
	{
	public:
		FromHttpServiceMethod1(const std::string & name, T* o, HttpFromMethod1<T> func)
				: HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
		{
		}

	public:
		int Invoke(const http::Request & request, http::Response & response) final
		{
			int code = XCode::Ok;
			const http::Content* body = request.GetBody();
			std::unique_ptr<json::w::Document> document2 = std::make_unique<json::w::Document>();
			do
			{
				const http::FromContent* fromData = (const http::FromContent*)body;
				std::unique_ptr<json::w::Value> data = document2->AddObject("data");
				if (fromData == nullptr)
				{
					const http::FromContent& fromData1 = request.GetUrl().GetQuery();
					code = (this->mObj->*mFunction)(fromData1, *data);
					break;
				}
				if (body->GetContentType() != http::ContentType::FROM)
				{
					return XCode::CallArgsError;
				}
				code = (this->mObj->*mFunction)(*fromData, *data);
			}
			while (false);
			if (code != XCode::Ok)
			{
				return code;
			}
			document2->Add("code", code);
			response.SetContent(*document2);
			return code;
		}

	private:
		T* mObj;
		HttpFromMethod1<T> mFunction;
	};

	template<typename T>
	class FromHttpServiceMethod2 : public HttpServiceMethod
	{
	public:
		FromHttpServiceMethod2(const std::string & name, T* o, HttpFromMethod2<T> func)
				: HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
		{
		}

	public:
		int Invoke(const http::Request & request, http::Response & response) final
		{
			const http::Content * body = request.GetBody();
			if(body != nullptr && body->GetContentType() != http::ContentType::FROM)
			{
				return XCode::CallArgsError;
			}
			if(body == nullptr)
			{
				const http::FromContent & fromData = request.GetUrl().GetQuery();
				return (this->mObj->*mFunction)(fromData, response);
			}
			const http::FromContent * fromData = body->To<const http::FromContent>();
			if(fromData == nullptr)
			{
				return XCode::CallArgsError;
			}
			return (this->mObj->*mFunction)(*fromData, response);
		}

	private:
		T* mObj;
		HttpFromMethod2<T> mFunction;
	};

	template<typename T>
	class FromHttpServiceMethod3 : public HttpServiceMethod
	{
	public:
		FromHttpServiceMethod3(const std::string & name, T* o, HttpFromMethod3<T> func)
				: HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
		{
		}

	public:
		int Invoke(const http::Request & request, http::Response & response) final
		{
			int code = XCode::Ok;
			const http::Content* body = request.GetBody();
			std::unique_ptr<json::w::Document> document2 = std::make_unique<json::w::Document>();
			do
			{
				if (body == nullptr)
				{
					const http::FromContent & fromData = request.GetUrl().GetQuery();
					code = (this->mObj->*mFunction)(fromData, *document2);
					break;
				}
				if (body->GetContentType() != http::ContentType::FROM)
				{
					return XCode::CallArgsError;
				}
				const http::FromContent* fromData = (const http::FromContent*)body;
				code = (this->mObj->*mFunction)(*fromData, *document2);
			} while (false);
			if(code != XCode::Ok)
			{
				return code;
			}
			document2->Add("code", code);
			response.SetContent(*document2);
			return code;
		}

	private:
		T* mObj;
		HttpFromMethod3<T> mFunction;
	};


	template<typename T>
	class FromHttpServiceMethod4 : public HttpServiceMethod
	{
	public:
		FromHttpServiceMethod4(const std::string & name, T* o, HttpFromMethod4<T> func)
				: HttpServiceMethod(name), mObj(o), mFunction(std::move(func))
		{
		}

	public:
		int Invoke(const http::Request & request, http::Response & response) final
		{
			std::unique_ptr<json::w::Document> document2 = std::make_unique<json::w::Document>();
			int code = (this->mObj->*mFunction)(request.ConstHeader(), *document2);
			if (code != XCode::Ok)
			{
				return code;
			}
			document2->Add("code", code);
			response.SetContent(*document2);
			return code;
		}

	private:
		T* mObj;
		HttpFromMethod4<T> mFunction;
	};


}
#endif //APP_HTTPSERVICEMETHOD_H
