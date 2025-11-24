//
// Created by yjz on 2022/1/22.
//

#ifndef APP_HTTPSERVICE_H
#define APP_HTTPSERVICE_H
#include"Entity/Component/Component.h"
#include"Rpc/Method/MethodRegister.h"
#include"Rpc/Config/ServiceConfig.h"
namespace json
{
	class Writer;
}
namespace http
{
	class Request;
	class Response;
}
namespace Lua
{
	class LuaModule;
}
namespace acs
{
	class HttpService : public Component
	{
	public:
		HttpService();
		~HttpService() override = default;
	protected:
		bool LateAwake() final;
		virtual bool OnInit() = 0;
	public:
		int Invoke(const HttpMethodConfig * config, const http::Request &, http::Response &);
	private:
		int CallLua(const std::string & method, const http::Request & request, http::Response & response);
		int AwaitCallLua(const std::string & method, const http::Request & request, http::Response & response);
	protected:
		Lua::LuaModule * mLuaModule;
		HttpServiceRegister mServiceRegister;
	};
}
#define BIND_COMMON_HTTP_METHOD(func) this->mServiceRegister.Bind(GET_FUNC_NAME(#func), &func)
#endif //APP_HTTPSERVICE_H
