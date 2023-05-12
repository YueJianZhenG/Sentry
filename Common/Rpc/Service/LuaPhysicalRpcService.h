﻿#pragma once


#include"RpcService.h"
#include"Lua/Engine/Define.h"
#include"Util/Json/JsonWriter.h"
#include"Rpc/Method/MethodRegister.h"

namespace Tendo
{
	class LuaScriptComponent;
	class ServiceMethodRegister;
	class LuaPhysicalRpcService : public RpcService, public IClient, public IServerRecord
			, public IEvent<DisConnectEvent>
	{
	 public:
		LuaPhysicalRpcService();
	 public:
		bool Init() final;
		bool Close() final;
		bool Start() final;
		bool LoadFromLua() final;
		void WaitAllMessageComplete() final;
		bool IsStartService() final { return true; }
		void OnRecord(Json::Writer &document) final;
		unsigned int GetWaitMessageCount() const final { return this->mWaitCount; };
		int Invoke(const std::string& name, std::shared_ptr<Msg::Packet> message) final;
	 private:
		void OnLogin(long long userId) final;
		void OnLogout(long long userId) final;
		void OnEvent(const DisConnectEvent *message) final;
    private:
		bool mIsHandlerMessage;
		unsigned int mSumCount;
		unsigned int mWaitCount;
		unsigned int mUserCount;
		ServiceMethodRegister mMethodRegister;
		class LuaScriptComponent* mLuaComponent;
	};
}// namespace Sentry