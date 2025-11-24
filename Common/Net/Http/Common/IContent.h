//
// Created by 64658 on 2024/10/22.
//

#ifndef APP_ICONTENT_H
#define APP_ICONTENT_H

#include"Http/Client/Http.h"

struct lua_State;
namespace http
{
	class Content
	{
	public:
		Content() = default;
		virtual ~Content() = default;
	public:
		virtual bool OnDecode() = 0;
		virtual void WriteToLua(lua_State * l) = 0;
		virtual std::string ToStr() const { return ""; }
		virtual void OnWriteHead(std::ostream & os) = 0;
		virtual int OnWriteBody(std::ostream & os) = 0;
		virtual int OnRecvMessage(std::istream & is, size_t size) = 0;
		virtual size_t ContentLength() = 0;
		virtual void SetContentLength(long long) { }
	public:
		template<typename T>
		T * Cast() { return static_cast<T*>(this); }
		template<typename T>
		const T * To() const { return dynamic_cast<T*>(this); }
	public:
		virtual http::ContentType GetContentType() const = 0;
	};
}


#endif //APP_ICONTENT_H
