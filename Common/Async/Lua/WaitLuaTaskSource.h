//
// Created by mac on 2022/3/31.
//

#ifndef APP_WAITLUATASKSOURCE_H
#define APP_WAITLUATASKSOURCE_H
#include"Lua/Engine/LuaParameter.h"
#include"Async/Source/TaskSource.h"
namespace acs
{
	// 在c++中等待lua完成
	class WaitLuaTaskSource final : public WaitTaskSourceBase
	{
	 public:
		explicit WaitLuaTaskSource();
		~WaitLuaTaskSource() final;
	public:
		template<typename T> T Await();
		static int SetResult(lua_State * lua);
	private:
		int luaRef;
		int valRef;
		lua_State * mLua;
	};

	template<typename T>
	T WaitLuaTaskSource::Await()
	{
		this->YieldTask();
		lua_rawgeti(mLua, LUA_REGISTRYINDEX, valRef);
		return Lua::Parameter::Read<T>(this->mLua, -1);
	}
}
#endif //APP_WAITLUATASKSOURCE_H
