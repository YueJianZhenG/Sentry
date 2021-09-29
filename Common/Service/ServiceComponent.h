﻿#pragma once

#include <memory>
#include <Component/Component.h>
#include <NetWork/PacketMapper.h>

using namespace std;
using namespace com;


namespace Sentry
{
	class ServiceMethod;
	class NetWorkWaitCorAction;

	class ServiceComponent : public Component
	{
	public:
		ServiceComponent() {}

		virtual ~ServiceComponent() {}

	public:

		virtual void Start() {};
		virtual bool Awake() { return true; }
		virtual int GetPriority() { return 1000; }
		virtual bool IsLuaService() { return false; };
	
	public:
		virtual void OnRefreshService() {}; //刷新服务表调用
		virtual const std::string &GetServiceName() = 0;
	public:
		bool AddMethod(ServiceMethod * method);
		bool HasMethod(const std::string &method);
		ServiceMethod * GetMethod(const std::string &method);
	private:	
		std::unordered_map<std::string, ServiceMethod *> mMethodMap;
		std::unordered_map<std::string, ServiceMethod *> mLuaMethodMap;
	};
}