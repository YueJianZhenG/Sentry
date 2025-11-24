#pragma once
#include "XCode/XCode.h"
#include "Rpc/Common/Message.h"
#include "Proto/Document/BsonDocument.h"

namespace acs
{
	template<typename T>
	using ServiceMethodType1 = int(T::*)();

	template<typename T>
	using ServiceMethodType11 = int(T::*)(long long);

	template<typename T, typename T1>
	using ServiceMethodType2 = int(T::*)(const T1 &);

	template<typename T, typename T1>
	using ServiceMethodType22 = int(T::*)(long long, const T1 &);

	template<typename T, typename T1, typename T2>
	using ServiceMethodType3 = int(T::*)(const T1 &, T2 &);

	template<typename T, typename T1, typename T2>
	using ServiceMethodType33 = int(T::*)(long long, const T1 &, T2 &);

	template<typename T, typename T1>
	using ServiceMethodType34 = int(T::*)(const T1 &, rpc::Message &);

	template<typename T, typename T1>
	using ServiceMethodType4 = int(T::*)(T1 &);

	template<typename T, typename T1>
	using ServiceMethodType44 = int(T::*)(long long, T1 &);

	template<typename T>
	using ServiceMethodType6 = int(T::*)(const rpc::Message & packet);

	template<typename T>
	using ServiceMethodType7 = int(T::*)(const rpc::Message& request, rpc::Message & response);

	template<typename T>
	using ServiceMethodType8 = int(T::*)(const json::r::Document& request);

	template<typename T>
	using ServiceMethodType88 = int(T::*)(long long id, const json::r::Document& request);

	template<typename T>
	using ServiceMethodType9 = int(T::*)(const json::r::Document& request, json::w::Document & response);

	template<typename T>
	using ServiceMethodType99 = int(T::*)(const json::r::Document& request, rpc::Message & response);

	template<typename T>
	using ServiceMethodType10 = int(T::*)(json::w::Document & response);

	template<typename T>
	using ServiceMethodType12 = int(T::*)(const std::string & request, json::w::Document & response);

	template<typename T>
	using ServiceMethodType101 = int(T::*)(const bson::r::Document& request);

	template<typename T>
	using ServiceMethodType102 = int(T::*)(const bson::r::Document& request, bson::w::Document & response);

//    template<typename T, typename T1>
//    using ServiceMethodType5 = int(T::*)(const Rpc::Head & head, const T1 &);
//
//
//    template<typename T>
//    using ServiceMethodType7 = int(T::*)(const Rpc::Head & head, Rpc::Head &);


//	template<typename T, typename T1, typename T2>
//	using ServiceMethodType55 = XCode(T::*)(const std::string & address, const T1 &, T2 &);

}// namespace Sentry


namespace acs
{

	class ServiceMethod
	{
	 public:
		explicit ServiceMethod(std::string name)
			: mName(std::move(name)) {}
			virtual ~ServiceMethod() = default;
	 public:
		virtual int Invoke(rpc::Message & message) = 0;
		inline const std::string& GetName() { return this->mName; }
	 private:
		std::string mName;
	};

	template<typename T>
	class ServiceMethod1 final : public ServiceMethod
	{
	 public:
		ServiceMethod1(const std::string name, T* o, ServiceMethodType1<T> func, std::string key)
			: ServiceMethod(name), _o(o), mHasUserId(false), mKey(std::move(key)), _func(func)
		{
		}

		ServiceMethod1(const std::string name, T* o, ServiceMethodType11<T> func, std::string key)
			: ServiceMethod(name), _o(o), mHasUserId(true), mKey(std::move(key)), _objfunc(func)
		{
		}
	 public:

		inline int Invoke(rpc::Message & message) final
		{
			int code = XCode::Ok;
			do
			{
				if (!this->mHasUserId)
				{
					code = (_o->*_func)();
					break;
				}
				long long userId = 0;
				if(!message.ConstHead().Get(this->mKey, userId))
				{
					code = XCode::CallArgsError;
					break;
				}
				code = (_o->*_objfunc)(userId);
			}
			while(false);
			message.ClearHeadAndBody();
			return code;
		}
	 private:
		T* _o;
		const bool mHasUserId;
		const std::string mKey;
		ServiceMethodType1<T> _func;
		ServiceMethodType11<T> _objfunc;
	};
	template<typename T, typename T1>
	class ServiceMethod2 final : public ServiceMethod
	{
	 public:
		ServiceMethod2(const std::string name, T* o, ServiceMethodType2<T, T1> func, std::string key)
			: ServiceMethod(name), _o(o),mHasUserId(false), mKey(std::move(key)), _func(func)
		{
		}

		ServiceMethod2(const std::string name, T* o, ServiceMethodType22<T, T1> func, std::string key)
			: ServiceMethod(name), _o(o), mHasUserId(true), mKey(std::move(key)), _objfunc(func) { }
	 public:
		inline int Invoke(rpc::Message & message) final
		{
			int code = XCode::Ok;
			do
			{
				std::unique_ptr<T1> request = std::make_unique<T1>();
				if(!request->ParsePartialFromString(message.GetBody()))
				{
					code = XCode::ParseMessageError;
					break;
				}
				if(!this->mHasUserId)
				{
					code = (_o->*_func)(*request);
					break;
				}
				long long userId = 0;
				if(!message.ConstHead().Get(this->mKey, userId))
				{
					code = XCode::CallArgsError;
					break;
				}
				code = (_o->*_objfunc)(userId, *request);
			}
			while (false);
			message.ClearHeadAndBody();
			return code;
		}
	 private:
		T* _o;
		const bool mHasUserId;
		const std::string mKey;
		ServiceMethodType2<T, T1> _func;
		ServiceMethodType22<T, T1> _objfunc;
	};

	template<typename T, typename T1, typename T2>
	class ServiceMethod3 final : public ServiceMethod
	{
	 public:
		typedef int(T::*ServerFunc)(long long, const T1&, T2&);
		ServiceMethod3(const std::string name, T* o, ServiceMethodType3<T, T1, T2> func, std::string key)
			: ServiceMethod(name), _o(o),mHasUserId(false), mKey(std::move(key)), _func(func)
		{
		}

		ServiceMethod3(const std::string name, T* o, ServiceMethodType33<T, T1, T2> func, std::string key)
			: ServiceMethod(name), _o(o), mHasUserId(true), mKey(std::move(key)), _objfunc(func)
		{
		}
	 public:
		inline int Invoke(rpc::Message & message) final
		{
			int code = XCode::Ok;
			std::unique_ptr<T1> request = std::make_unique<T1>();
			std::unique_ptr<T2> response = std::make_unique<T2>();
			do
			{
				if(!request->ParsePartialFromString(message.GetBody()))
				{
					code = XCode::ParseMessageError;
					break;
				}
				if (!this->mHasUserId)
				{
					code = (_o->*_func)(*request, *response);
					break;
				}
				long long userId = 0;
				if (!message.ConstHead().Get(this->mKey, userId))
				{
					code = XCode::CallArgsError;
					break;
				}
				code = (_o->*_objfunc)(userId, *request, *response);
			}
			while(false);
			message.ClearHeadAndBody();
			if(code == XCode::Ok)
			{
				message.SetProto(rpc::proto::pb);
				response->SerializePartialToString(message.Body());
			}
			return code;
		}
	 private:
		T* _o;
		const bool mHasUserId;
		const std::string mKey;
		ServiceMethodType3<T, T1, T2> _func;
		ServiceMethodType33<T, T1, T2> _objfunc;
	};

	template<typename T, typename T1>
	class ServiceMethod4 final : public ServiceMethod
	{
	 public:
		ServiceMethod4(const std::string name, T* o, ServiceMethodType4<T, T1> func, std::string key)
			: ServiceMethod(name), _o(o),mHasUserId(false), mKey(std::move(key)), _func(func)
		{
		}

		ServiceMethod4(const std::string name, T* o, ServiceMethodType44<T, T1> func, std::string key)
			: ServiceMethod(name), _o(o), mHasUserId(true), mKey(std::move(key)), _objfunc(func)
		{
		}
	 public:
		inline int Invoke(rpc::Message & message) final
		{
			int code = XCode::Ok;
			std::unique_ptr<T1> response = std::make_unique<T1>();
			do
			{
				if(!this->mHasUserId)
				{
					code = (_o->*_func)(*response);
					break;
				}
				long long userId = 0;
				if(!message.ConstHead().Get(this->mKey, userId))
				{
					code = XCode::CallArgsError;
					break;
				}
				code = (_o->*_objfunc)(userId, *response);
			}
			while(false);
			message.ClearHeadAndBody();
			if(code == XCode::Ok)
			{
				message.SetProto(rpc::proto::pb);
				response->SerializePartialToString(message.Body());
			}
			return code;
		}
	 private:
		T* _o;
		const bool mHasUserId;
		const std::string mKey;
		ServiceMethodType4<T, T1> _func;
		ServiceMethodType44<T, T1> _objfunc;
	};

	template<typename T>
	class ServiceMethod6 final : public ServiceMethod
	{
	 public:
		ServiceMethod6(const std::string name, T* o, ServiceMethodType6<T> func, std::string key)
			: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	 public:
		inline int Invoke(rpc::Message & message) final
		{
			return (_o->*_func)(message);
		}
	 private:
		T* _o;
		const std::string mKey;
		ServiceMethodType6<T> _func;
	};

	template<typename T>
	class ServiceMethod7 final : public ServiceMethod
	{
	public:
		ServiceMethod7(const std::string name, T* o, ServiceMethodType7<T> func, std::string key)
			: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			return (_o->*_func)(message, message);
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType7<T> _func;
	};

	template<typename T>
	class ServiceMethod8 final : public ServiceMethod
	{
	public:
		ServiceMethod8(const std::string name, T* o, ServiceMethodType8<T> func, std::string key)
				: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			int code = XCode::Ok;
			std::unique_ptr<json::r::Document> request = std::make_unique<json::r::Document>();
			do
			{
				const std::string & json = message.GetBody();
				if(!request->Decode(json, YYJSON_READ_INSITU))
				{
					code = XCode::ParseJsonFailure;
					break;
				}
				code = (_o->*_func)(*request);
			}
			while(false);
			message.ClearHeadAndBody();
			return code;
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType8<T> _func;
	};

	template<typename T>
	class ServiceMethod88 final : public ServiceMethod
	{
	public:
		ServiceMethod88(const std::string name, T* o, ServiceMethodType88<T> func, std::string key)
				: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			int code = XCode::Ok;
			std::unique_ptr<json::r::Document> request = std::make_unique<json::r::Document>();
			do
			{
				const std::string & json = message.GetBody();
				if(!request->Decode(json, YYJSON_READ_INSITU))
				{
					code = XCode::ParseJsonFailure;
					break;
				}
				long long userId = 0;
				if(!message.ConstHead().Get(this->mKey, userId))
				{
					code = XCode::CallArgsError;
					break;
				}
				code = (_o->*_func)(userId, *request);
			}
			while(false);
			message.ClearHeadAndBody();
			return code;
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType88<T> _func;
	};

	template<typename T>
	class ServiceMethod101 final : public ServiceMethod
	{
	public:
		ServiceMethod101(const std::string name, T* o, ServiceMethodType101<T> func, std::string key)
				: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			int code = XCode::Ok;
			const std::string & body = message.GetBody();
			std::unique_ptr<bson::r::Document> request = std::make_unique<bson::r::Document>();
			do
			{
				request->Init(body.c_str());
				code = (_o->*_func)(*request);
			}
			while(false);
			message.ClearHeadAndBody();
			return code;
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType101<T> _func;
	};

	template<typename T>
	class ServiceMethod9 final : public ServiceMethod
	{
	public:
		ServiceMethod9(const std::string name, T* o, ServiceMethodType9<T> func, std::string key)
				: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			int code = XCode::Ok;
			std::unique_ptr<json::r::Document> request = std::make_unique<json::r::Document>();
			std::unique_ptr<json::w::Document> response = std::make_unique<json::w::Document>();
			do
			{
				const std::string & json = message.GetBody();
				if(!request->Decode(json, YYJSON_READ_INSITU))
				{
					code = XCode::ParseJsonFailure;
					break;
				}
				code = (_o->*_func)(*request, *response);
			}
			while(false);
			message.ClearHeadAndBody();
			if(code == XCode::Ok)
			{
				message.SetProto(rpc::proto::json);
				response->Serialize(message.Body());
			}
			return code;
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType9<T> _func;
	};

	template<typename T>
	class ServiceMethod99 final : public ServiceMethod
	{
	public:
		ServiceMethod99(const std::string name, T* o, ServiceMethodType99<T> func, std::string key)
				: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			int code = XCode::Ok;
			std::unique_ptr<json::r::Document> request = std::make_unique<json::r::Document>();
			do
			{
				const std::string & json = message.GetBody();
				if(!request->Decode(json, YYJSON_READ_INSITU))
				{
					code = XCode::ParseJsonFailure;
					break;
				}
				code = (_o->*_func)(*request, message);
			}
			while(false);
			return code;
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType99<T> _func;
	};

	template<typename T>
	class ServiceMethod102 final : public ServiceMethod
	{
	public:
		ServiceMethod102(const std::string name, T* o, ServiceMethodType102<T> func, std::string key)
				: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			int code = 0;
			int count = 0;
			const std::string& body = message.GetBody();
			std::unique_ptr<bson::r::Document> request = std::make_unique<bson::r::Document>();
			std::unique_ptr<bson::w::Document> response = std::make_unique<bson::w::Document>();
			{
				request->Init(body.c_str());
				code = (_o->*_func)(*request, *response);
				{
					message.ClearHeadAndBody();
					const char * bson = response->Serialize(count);
					message.SetContent(rpc::proto::bson, bson, count);
				}

			}
			return code;
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType102<T> _func;
	};

	template<typename T>
	class ServiceMethod10 final : public ServiceMethod
	{
	public:
		ServiceMethod10(const std::string name, T* o, ServiceMethodType10<T> func, std::string key)
				: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			std::unique_ptr<json::w::Document> response = std::make_unique<json::w::Document>();
			{
				int code = (_o->*_func)(*response);
				{
					message.ClearHeadAndBody();
					if(code == XCode::Ok)
					{
						message.SetProto(rpc::proto::json);
						response->Serialize(message.Body());
					}
				}
				return code;
			}
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType10<T> _func;
	};

	template<typename T>
	class ServiceMethod12 final : public ServiceMethod
	{
	public:
		ServiceMethod12(const std::string name, T* o, ServiceMethodType12<T> func, std::string key)
				: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			std::unique_ptr<json::w::Document> response = std::make_unique<json::w::Document>();
			{
				int code = (_o->*_func)(message.GetBody(), *response);
				{
					message.ClearHeadAndBody();
					if(code == XCode::Ok)
					{
						message.SetProto(rpc::proto::json);
						response->Serialize(message.Body());
					}
				}
				return code;
			}
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType12<T> _func;
	};

	template<typename T, typename T1>
	class ServiceMethod34 final : public ServiceMethod
	{
	public:
		ServiceMethod34(const std::string name, T* o, ServiceMethodType34<T, T1> func, std::string key)
				: ServiceMethod(name), _o(o), mKey(std::move(key)), _func(func)
		{

		}

	public:
		inline int Invoke(rpc::Message& message) final
		{
			std::unique_ptr<T1> request = std::make_unique<T1>();
			if(!request->ParsePartialFromString(message.GetBody()))
			{
				return XCode::ParseMessageError;
			}
			return (_o->*_func)(*request, message);
		}
	private:
		T* _o;
		std::string mKey;
		ServiceMethodType34<T, T1> _func;
	};
}