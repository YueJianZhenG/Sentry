﻿#pragma once
#include"XCode/XCode.h"
#include"Rpc/Client/Message.h"
#include"Util/Json/JsonReader.h"
using namespace google::protobuf;
namespace Tendo
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
	using ServiceMethodType4 = int(T::*)(T1 &);

	template<typename T, typename T1>
	using ServiceMethodType44 = int(T::*)(long long, T1 &);

	template<typename T>
	using ServiceMethodType6 = int(T::*)(const Rpc::Packet & packet);

	template<typename T>
	using ServiceMethodType7 = int(T::*)(const Rpc::Packet& request, Rpc::Packet & response);

	template<typename T, typename T1>
	using ServiceMethodType8 = int(T::*)(const std::string & address, const T1&);

//    template<typename T, typename T1>
//    using ServiceMethodType5 = int(T::*)(const Rpc::Head & head, const T1 &);
//
//
//    template<typename T>
//    using ServiceMethodType7 = int(T::*)(const Rpc::Head & head, Rpc::Head &);


//	template<typename T, typename T1, typename T2>
//	using ServiceMethodType55 = XCode(T::*)(const std::string & address, const T1 &, T2 &);

}// namespace Sentry


namespace Tendo
{

	class ServiceMethod
	{
	 public:
		explicit ServiceMethod(std::string name)
			: mName(std::move(name)) {}
	 public:
		virtual bool IsLuaMethod() = 0;
		virtual int Invoke(Rpc::Packet & message) = 0;
		const std::string& GetName()
		{
			return this->mName;
		}
	 private:
		std::string mName;
	};

	template<typename T>
	class ServiceMethod1 : public ServiceMethod
	{
	 public:
		ServiceMethod1(const std::string name, T* o, ServiceMethodType1<T> func)
			: ServiceMethod(name), _o(o), mHasUserId(false), _func(func)
		{
		}

		ServiceMethod1(const std::string name, T* o, ServiceMethodType11<T> func)
			: ServiceMethod(name), _o(o), mHasUserId(true), _objfunc(func)
		{
		}
	 public:

		int Invoke(Rpc::Packet & message) final
		{
			if (!this->mHasUserId)
			{
				return (_o->*_func)();
			}
            long long userId = 0;
            if(!message.GetHead().Get("id", userId))
            {
                return XCode::CallArgsError;
            }
			return (_o->*_objfunc)(userId);
		}

		bool IsLuaMethod() override
		{
			return false;
		};
	 private:
		T* _o;
		const bool mHasUserId;
		ServiceMethodType1<T> _func;
		ServiceMethodType11<T> _objfunc;
	};
	template<typename T, typename T1>
	class ServiceMethod2 : public ServiceMethod
	{
	 public:
		ServiceMethod2(const std::string name, T* o, ServiceMethodType2<T, T1> func)
			: ServiceMethod(name), _o(o),mHasUserId(false), _func(func)
		{
		}

		ServiceMethod2(const std::string name, T* o, ServiceMethodType22<T, T1> func)
			: ServiceMethod(name), _o(o), mHasUserId(true), _objfunc(func)
		{
		}
	 public:
		int Invoke(Rpc::Packet & message) override
		{
            std::unique_ptr<T1> request(new T1());
            if(!message.ParseMessage(request.get()))
            {
                return XCode::CallArgsError;
            }
            message.Clear();
            if (!this->mHasUserId)
			{
				return (_o->*_func)(*request);
			}
            long long userId = 0;
            if(!message.GetHead().Get("id", userId))
            {
                return XCode::CallArgsError;
            }
			return (_o->*_objfunc)(userId, *request);
		}
		bool IsLuaMethod() override
		{
			return false;
		};
	 private:
		T* _o;
		const bool mHasUserId;
		ServiceMethodType2<T, T1> _func;
		ServiceMethodType22<T, T1> _objfunc;
	};

	template<typename T, typename T1, typename T2>
	class ServiceMethod3 : public ServiceMethod
	{
	 public:
		typedef int(T::*ServerFunc)(long long, const T1&, T2&);
		ServiceMethod3(const std::string name, T* o, ServiceMethodType3<T, T1, T2> func)
			: ServiceMethod(name), _o(o),mHasUserId(false), _func(func)
		{
		}

		ServiceMethod3(const std::string name, T* o, ServiceMethodType33<T, T1, T2> func)
			: ServiceMethod(name), _o(o), mHasUserId(true), _objfunc(func)
		{
		}
	 public:
		int Invoke(Rpc::Packet & message) override
		{
            std::unique_ptr<T1> request(new T1());
            std::unique_ptr<T2> response(new T2());
            if(!message.ParseMessage(request.get()))
            {
                return XCode::CallArgsError;
            }
            message.Clear();
            if (this->mHasUserId)
            {
                long long userId = 0;
                if (!message.GetHead().Get("id", userId))
                {
                    return XCode::CallArgsError;
                }
                int code = (_o->*_objfunc)(userId, *request, *response);
                if (code == XCode::Successful)
                {
                    if (!message.WriteMessage(response.get()))
                    {
                        return XCode::SerializationFailure;
                    }
                    return XCode::Successful;
                }
                return code;
            }
			int code = (_o->*_func)(*request, *response);
			if (code == XCode::Successful)
			{
                if(!message.WriteMessage(response.get()))
                {
                    return XCode::SerializationFailure;
                }
                return XCode::Successful;
			}
			return code;
		}
		bool IsLuaMethod() override
		{
			return false;
		};
	 private:
		T* _o;
		const bool mHasUserId;
		ServiceMethodType3<T, T1, T2> _func;
		ServiceMethodType33<T, T1, T2> _objfunc;
	};

	template<typename T, typename T1>
	class ServiceMethod4 : public ServiceMethod
	{
	 public:
		ServiceMethod4(const std::string name, T* o, ServiceMethodType4<T, T1> func)
			: ServiceMethod(name), _o(o),mHasUserId(false), _func(func)
		{
		}

		ServiceMethod4(const std::string name, T* o, ServiceMethodType44<T, T1> func)
			: ServiceMethod(name), _o(o), mHasUserId(true), _objfunc(func)
		{
		}
	 public:
		int Invoke(Rpc::Packet & message) override
		{
			std::unique_ptr<T1> response(new T1());
            if (this->mHasUserId)
			{
                long long userId = 0;
                if(!message.GetHead().Get("id", userId))
                {
                    return XCode::CallArgsError;
                }
				int code = (_o->*_objfunc)(userId, *response);
				if(code == XCode::Successful)
				{
					message.WriteMessage(response.get());
					return XCode::Successful;
				}
				return code;
			}
			int code = (_o->*_func)(*response);
			if(code == XCode::Successful)
			{
				message.WriteMessage(response.get());
				return XCode::Successful;
			}
			return code;
		}
		bool IsLuaMethod() override
		{
			return false;
		};
	 private:
		T* _o;
		const bool mHasUserId;
		ServiceMethodType4<T, T1> _func;
		ServiceMethodType44<T, T1> _objfunc;
	};


//	template<typename T, typename T1>
//	class ServiceMethod5 : public ServiceMethod
//	{
//	public:
//		ServiceMethod5(const std::string name, T* o, ServiceMethodType5<T, T1> func)
//				: ServiceMethod(name), _o(o), _func(func)
//		{
//
//		}
//
//	public:
//		bool IsLuaMethod() override
//		{
//			return false;
//		};
//		int Invoke(Rpc::Packet & message) override
//		{
//			std::unique_ptr<T1> request(new T1());
//            if(!message.ParseMessage(request.get()))
//            {
//                return XCode::CallArgsError;
//            }
//            message.Clear();
//			return (_o->*_func)(message.GetHead(), *request);
//		}
//
//	private:
//		T* _o;
//		ServiceMethodType5<T, T1> _func;
//	};

	template<typename T>
	class ServiceMethod6 : public ServiceMethod
	{
	 public:
		ServiceMethod6(const std::string name, T* o, ServiceMethodType6<T> func)
			: ServiceMethod(name), _o(o), _func(func)
		{

		}

	 public:
		bool IsLuaMethod() override { return false; };
		int Invoke(Rpc::Packet & message) override
		{
			return (_o->*_func)(message);
		}
	 private:
		T* _o;
		ServiceMethodType6<T> _func;
	};

	template<typename T>
	class ServiceMethod7 : public ServiceMethod
	{
	public:
		ServiceMethod7(const std::string name, T* o, ServiceMethodType7<T> func)
			: ServiceMethod(name), _o(o), _func(func)
		{

		}

	public:
		bool IsLuaMethod() override { return false; };
		int Invoke(Rpc::Packet& message) override
		{
			return (_o->*_func)(message, message);
		}
	private:
		T* _o;
		ServiceMethodType7<T> _func;
	};

	template<typename T, typename T1>
	class ServiceMethod8 : public ServiceMethod
	{
	public:
		ServiceMethod8(const std::string name, T* o, ServiceMethodType8<T,T1> func)
			: ServiceMethod(name), _o(o), _func(func)
		{

		}

	public:
		bool IsLuaMethod() override { return false; };
		int Invoke(Rpc::Packet& message) override
		{
			std::unique_ptr<T1> request(new T1());
			const std::string& address = message.From();
			if (!message.ParseMessage(request.get()))
			{
				return XCode::CallArgsError;
			}
			return (_o->*_func)(address, *request);
		}
	private:
		T* _o;
		ServiceMethodType8<T, T1> _func;
	};
}