#pragma once

#include <string>
#include <unordered_set>
namespace acs
{
    class MethodConfig
    {
    public:
    	virtual ~MethodConfig() = default;
    public:
		bool lock; //是否使用分布式锁
		bool async;
		bool record;
    	int timeout; //调用超时 秒
		std::string method;
		std::string headers;
		std::string node; //所在节点
		std::string service; //服务名
	};

	class RpcMethodConfig final : public MethodConfig
	{
	public:
		int opcode = 0;
		char net; //使用的网络
		char proto;
		char forward;	//转发策略 0固定转发，1随机转发到某一个机器
		bool open;
		bool auth;
        bool debug;
		bool client;  //是否能被客户端调用
		bool to_client;	//消息是否发送到客户端
		std::string desc;
		std::string request;
		std::string response;
		std::string fullname;
	public:
		std::string NetName;
		std::string ProtoName;
		std::string ForwardName;
	};

	class HttpMethodConfig final : public MethodConfig
	{
	public:
		bool auth;
		int limit;
		bool open;
		int timeout;
		int access;
		std::string path;
		std::string type;
		std::string desc;
		std::string token;
		std::string content;
		std::string request;
		std::string upload;
		std::unordered_set<std::string> WhiteList;
	};

}// namespace Sentry