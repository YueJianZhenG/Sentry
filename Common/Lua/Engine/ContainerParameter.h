
#pragma once
#include "LuaParameter.h"
namespace Lua
{
	namespace ContainerParameter
	{
		template<typename T>
		struct ContainerStruct
		{
			static T Read(lua_State* lua, int index)
			{
				assert(false);
				return T();
			}

			static void Write(lua_State* lua, T data)
			{
				assert(false);
			}
		};

		template<typename T>
		inline T Read(lua_State* lua, int index)
		{
			assert(lua_istable(lua, index));
			return ContainerStruct<T>::Read(lua, index);
		}

		template<typename T>
		inline void Write(lua_State* lua, T data)
		{
			ContainerStruct<T>::Write(lua, data);
		}
	}// namespace ContainerParameter

// vector 对象
	namespace ContainerParameter
	{
		template<typename T>
		struct ContainerStruct<std::vector<T>>
		{
			static std::vector<T> Read(lua_State* lua, int index)
			{
				assert(lua_istable(lua, index));
				std::vector<T> ret;
				lua_pushnil(lua);
				while (lua_next(lua, -2) != 0)
				{
					ret.emplace_back(Parameter::Read<T>(lua, -1));
					lua_pop(lua, 1);
				}
				return ret;
			}

			static void Write(lua_State* lua, std::vector<T> data)
			{
				lua_newtable(lua);
				int top = lua_gettop(lua);
				for (size_t index = 0; index < data.size(); index++)
				{
					lua_pushinteger(lua, index + 1);
					const T value = data.at(index);
					Parameter::Write(lua, value);
					lua_settable(lua, -3);
				}
				lua_settop(lua, top);
			}
		};
	}// namespace ContainerParameter

// vector 引用
	namespace ContainerParameter
	{
		template<typename T>
		struct ContainerStruct<std::vector<T>&>
		{
			//读的时候注意释放内存
//			static std::vector<T>& Read(lua_State* lua, int index)
//			{
//				assert(false);
//				return std::vector<T>();
//			}

			static void Write(lua_State* lua, std::vector<T>& data)
			{
				lua_newtable(lua);
				int top = lua_gettop(lua);
				for (size_t index = 0; index < data.size(); index++)
				{
					lua_pushinteger(lua, index);
					const T value = data.at(index);
					Parameter::Write(lua, value);
					lua_settable(lua, -3);
				}
				lua_settop(lua, top);
			}
		};
	}// namespace ContainerParameter

// vector 指针
	namespace ContainerParameter
	{
		template<typename T>
		struct ContainerStruct<std::vector<T>*>
		{
			//读的时候注意释放内存
			static std::shared_ptr<std::vector<T>> Read(lua_State* lua, int index)
			{
				assert(lua_istable(lua, index));
				std::vector<T> ret = new std::vector<T>();
				lua_pushnil(lua);
				while (lua_next(lua, index) != 0)
				{
					const T value = Parameter::Read<T>(lua, -1);
					ret->push_back(value);
					lua_pop(lua, 1);
				}
				return ret;
			}

			static void Write(lua_State* lua, std::vector<T>* data)
			{
				lua_newtable(lua);
				int top = lua_gettop(lua);
				for (size_t index = 0; index < data->size(); index++)
				{
					lua_pushinteger(lua, index);
					const T value = data->at(index);
					Parameter::Write(lua, value);
					lua_settable(lua, -3);
				}
				lua_settop(lua, top);
			}
		};
	}// namespace ContainerParameter

// hashmap对象
	namespace ContainerParameter
	{
		template<typename Key, typename Value>
		struct ContainerStruct<std::unordered_map<Key, Value>>
		{
			static std::unordered_map<Key, Value> Read(lua_State* lua, int index)
			{
				assert(lua_istable(lua, index));
				std::unordered_map<Key, Value> ret;
				lua_pushnil(lua);
				while (lua_next(lua, -2) != 0)
				{
					const Key key = Parameter::Read<Key>(lua, -2);
					const Value value = Parameter::Read<Value>(lua, -1);
					ret.emplace(key, value);
					lua_pop(lua, 1);
				}
				return ret;
			}

			static void Write(lua_State* lua, std::unordered_map<Key, Value> data)
			{
				lua_newtable(lua);
				int top = lua_gettop(lua);
				typedef typename std::unordered_map<Key, Value>::iterator MapIterator;
				MapIterator iter = data.begin();
				for (; iter != data.end(); ++iter)
				{
					Parameter::Write<Key>(lua, iter->first);
					Parameter::Write<Value>(lua, iter->second);
					lua_settable(lua, -3);
				}
				lua_settop(lua, top);
			}
		};
	}// namespace ContainerParameter

// hashmap引用
	namespace ContainerParameter
	{
		template<typename Key, typename Value>
		struct ContainerStruct<std::unordered_map<Key, Value>&>
		{
			static std::unordered_map<Key, Value> Read(lua_State* lua, int index)
			{
				assert(false);
			}

			static void Write(lua_State* lua, std::unordered_map<Key, Value>& data)
			{
				lua_newtable(lua);
				int top = lua_gettop(lua);
				typedef typename std::unordered_map<Key, Value>::iterator MapIterator;
				MapIterator iter = data.begin();
				for (; iter != data.end(); ++iter)
				{
					Parameter::Write<Key>(lua, iter->first);
					Parameter::Write<Value>(lua, iter->second);
					lua_settable(lua, -3);
				}
				lua_settop(lua, top);
			}
		};
	}// namespace ContainerParameter

// hashmap指针
	namespace ContainerParameter
	{
		template<typename Key, typename Value>
		struct ContainerStruct<std::unordered_map<Key, Value>*>
		{
			static std::unordered_map<Key, Value> Read(lua_State* lua, int index)
			{
				assert(false);
			}

			static void Write(lua_State* lua, std::unordered_map<Key, Value>* data)
			{
				lua_newtable(lua);
				int top = lua_gettop(lua);
				typedef typename std::unordered_map<Key, Value>::iterator MapIterator;
				MapIterator iter = data->begin();
				for (; iter != data->end(); ++iter)
				{
					Parameter::Write<Key>(lua, iter->first);
					Parameter::Write<Value>(lua, iter->second);
					lua_settable(lua, -3);
				}
				lua_settop(lua, top);
			}
		};
	}// namespace ContainerParameter

// map对象
	namespace ContainerParameter
	{
		template<typename Key, typename Value>
		struct ContainerStruct<std::map<Key, Value>>
		{
			static std::map<Key, Value> Read(lua_State* lua, int index)
			{
				assert(lua_istable(lua, index));
				std::map<Key, Value> ret;
				lua_pushnil(lua);
				while (lua_next(lua, -2) != 0)
				{
					const Key key = Parameter::Read<Key>(lua, -2);
					const Value value = Parameter::Read<Value>(lua, -1);
					ret.insert(std::make_pair(key, value));
					lua_pop(lua, 1);
				}
				return ret;
			}

			static void Write(lua_State* lua, std::map<Key, Value> data)
			{
				lua_newtable(lua);
				int top = lua_gettop(lua);
				typedef typename std::map<Key, Value>::iterator MapIterator;
				MapIterator iter = data.begin();
				for (; iter != data.end(); ++iter)
				{
					Parameter::Write<Key>(lua, iter->first);
					Parameter::Write<Value>(lua, iter->second);
					lua_settable(lua, -3);
				}
				lua_settop(lua, top);
			}
		};
	}// namespace ContainerParameter

// map引用
	namespace ContainerParameter
	{
		template<typename Key, typename Value>
		struct ContainerStruct<std::map<Key, Value>&>
		{
			static std::map<Key, Value>& Read(lua_State* lua, int index)
			{
				assert(lua_istable(lua, index));
				std::map<Key, Value>* ret = new std::map<Key, Value>();
				lua_pushnil(lua);
				while (lua_next(lua, -2) != 0)
				{
					lua_type(lua, -1);
					const Key key = Parameter::Read<Key>(lua, -2);
					const Value value = Parameter::Read<Value>(lua, -1);
					ret->insert(std::make_pair(key, value));
					lua_pop(lua, 1);
				}
				return *ret;
			}

			static void Write(lua_State* lua, std::map<Key, Value>& data)
			{
				lua_newtable(lua);
				int top = lua_gettop(lua);
				typedef typename std::map<Key, Value>::iterator MapIterator;
				MapIterator iter = data.begin();
				for (; iter != data.end(); ++iter)
				{
					Parameter::Write<Key>(lua, iter->first);
					Parameter::Write<Value>(lua, iter->second);
					lua_settable(lua, -3);
				}
				lua_settop(lua, top);
			}
		};
	}// namespace ContainerParameter

// map指针
	namespace ContainerParameter
	{
		template<typename Key, typename Value>
		struct ContainerStruct<std::map<Key, Value>*>
		{
			// 注意释放内存
			static std::map<Key, Value>* Read(lua_State* lua, int index)
			{
				assert(lua_istable(lua, index));
				std::map<Key, Value>* ret = new std::map<Key, Value>();
				lua_pushnil(lua);
				while (lua_next(lua, -2) != 0)
				{
					lua_type(lua, -1);
					const Key key = Parameter::Read<Key>(lua, -2);
					const Value value = Parameter::Read<Value>(lua, -1);
					ret->insert(std::make_pair(key, value));
					lua_pop(lua, 1);
				}
				return ret;
			}

			static void Write(lua_State* lua, std::map<Key, Value>* data)
			{
				lua_newtable(lua);
				int top = lua_gettop(lua);
				typedef typename std::map<Key, Value>::iterator MapIterator;
				MapIterator iter = data->begin();
				for (; iter != data->end(); ++iter)
				{
					Parameter::Write<Key>(lua, iter->first);
					Parameter::Write<Value>(lua, iter->second);
					lua_settable(lua, -3);
				}
				lua_settop(lua, top);
			}
		};
	}// namespace ContainerParameter
}