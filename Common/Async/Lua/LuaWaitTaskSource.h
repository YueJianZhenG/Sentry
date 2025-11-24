//
// Created by mac on 2022/5/31.
//

#ifndef APP_LUAWAITTASKSOURCE_H
#define APP_LUAWAITTASKSOURCE_H

#include"Lua/Engine/LuaParameter.h"
#include"Async/Lua/LuaCoroutine.h"
#include"Rpc/Common/Message.h"
#ifdef __SHARE_PTR_COUNTER__
#include "Core/Memory/MemoryObject.h"
#endif
namespace acs
{
	// 在lua中等待c++协程
	class LuaWaitTaskSource final
#ifdef __SHARE_PTR_COUNTER__
	: public memory::Object<LuaWaitTaskSource>
#endif
	{
	public:
		explicit LuaWaitTaskSource(lua_State* lua);

		~LuaWaitTaskSource();

	public:
		int Await();

		void SetResult();

		template<typename T>
		inline void SetResult(T result);

		template<typename T>
		inline void SetResultPtr(T * result);

		inline void SetError(const char * error);
		void SetResult(int code, std::unique_ptr<rpc::Message>& response);

		template<typename... Args>
		inline void SetResults(Args &&... args)
		{
			size_t size = sizeof...(Args);
			Lua::Parameter::WriteArgs(this->mLua, std::forward<Args>(args)...);
			Lua::Coroutine::Resume(this->mLua, (int)size);
		}
	 private:
		int mRef;
		lua_State* mLua;
	};

	template<typename T>
	inline void LuaWaitTaskSource::SetResult(T result)
	{
		Lua::Parameter::Write(this->mLua, result);
        Lua::Coroutine::Resume(this->mLua, 1);
	}

	template<typename T>
	inline void LuaWaitTaskSource::SetResultPtr(T* result)
	{
		Lua::UserDataParameter::UserDataStruct<T*>::WritePtr(this->mLua, result);
		Lua::Coroutine::Resume(this->mLua, 1);
	}

	void LuaWaitTaskSource::SetError(const char* error)
	{
		Lua::Coroutine::Resume(this->mLua, 0);
	}
}
#endif //APP_LUAWAITTASKSOURCE_H
