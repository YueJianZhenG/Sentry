#pragma once

#include"TimerBase.h"
#include"Lua/Engine/Define.h"
#include"Rpc/Method/MethodProxy.h"
namespace acs
{
	class DelayTimer final : public TimerBase
	{
	public:
		DelayTimer(long long id, int ms, std::unique_ptr<StaticMethod> func)
			: TimerBase(id, ms), mFunc(std::move(func)) {}
	public:
		void Invoke() final { this->mFunc->run(); }
	private:
		std::unique_ptr<StaticMethod> mFunc;
	};
}

namespace acs
{
	class LuaTimer final : public TimerBase
	{
	public:
		LuaTimer(long long id, lua_State * lua, int ref, int ms, int cor)
			: TimerBase(id, ms), ref(ref), cor(cor), mLua(lua) { }
		LuaTimer(long long id, lua_State * lua, int ref, std::string func, int ms, int cor)
				: TimerBase(id, ms), ref(ref), cor(cor), mMethod(std::move(func)), mLua(lua) { }
		~LuaTimer() override;
	public:
		void Invoke() final;
	private:
		int ref;
		int cor;
		lua_State * mLua;
		std::string mMethod;
	};
}