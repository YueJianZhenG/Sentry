#pragma once

#include <vector>
#include <string>
#include "Define.h"
#include <unordered_map>
namespace lua
{
	class MemberBase
	{
	public:
		virtual ~MemberBase() = default;
	public:
		virtual int Get(lua_State * L) = 0;
		virtual int Set(lua_State * L) = 0;
	};


}

namespace lua
{
	template<typename T>
	struct ClassFactory
	{
	public:
		static std::string name;
		static std::vector<std::string> parents;
		static std::unordered_map<std::string, MemberBase *> members;
	};

	template<typename T>
	std::string ClassFactory<T>::name;

	template<typename T>
	std::vector<std::string> ClassFactory<T>::parents;

	template<typename T>
	std::unordered_map<std::string, MemberBase *> ClassFactory<T>::members;
}
