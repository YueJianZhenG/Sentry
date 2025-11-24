#include "IProto.h"
#include <algorithm>
#include "Util/Tools/Math.h"
#include "Util/Tools/TimeHelper.h"
#include "Yyjson/Document/Document.h"
#include "Lua/Engine/UserDataParameter.h"

namespace tcp
{
	IProto::IProto()
	{
#ifdef __DEBUG__
		this->mStartTime = help::Time::NowTimeMS;
#endif
	}

#ifdef __DEBUG__
	long long IProto::GetCostTime() const
	{
		return help::Time::NowTimeMS - this->mStartTime;
	}
#endif
}

namespace tcp
{
	bool IHeader::Get(const std::string& k, int& v) const
	{
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
				[&k](const std::pair<std::string, std::string>& item)
				{
					return item.first == k;
				});
		if (iter == this->mHeader.end())
		{
			return false;
		}
		return help::Math::ToNumber(iter->second, v);
	}

	int IHeader::GetInt(const std::string& key, int def) const
	{
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
				[&key](const std::pair<std::string, std::string>& item)
				{
					return item.first == key;
				});
		if (iter == this->mHeader.end())
		{
			return def;
		}
		int result = 0;
		if(!help::Math::ToNumber(iter->second, result))
		{
			return def;
		}
		return result;
	}


	bool IHeader::Has(const std::string& k) const
	{
		return std::any_of(this->mHeader.begin(), this->mHeader.end(),
				[&k](const std::pair<std::string, std::string>& item)
				{
					return item.first == k;
				});
	}

	bool IHeader::Get(const std::string& k, long long& v) const
	{
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
				[&k](const std::pair<std::string, std::string>& item)
				{
					return item.first == k;
				});
		if (iter == this->mHeader.end())
		{
			return false;
		}
		return help::Math::ToNumber(iter->second, v);
	}

	bool IHeader::Get(const std::string& k, std::string& v) const
	{
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
				[&k](const std::pair<std::string, std::string>& item)
				{
					return item.first == k;
				});
		if (iter == this->mHeader.end())
		{
			return false;
		}
		v.assign(iter->second);
		return true;
	}


	bool IHeader::IsEqual(const std::string& k, const std::string& v) const
	{
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
				[&k](const std::pair<std::string, std::string>& item)
				{
					return item.first == k;
				});
		if (iter == this->mHeader.end())
		{
			return false;
		}
		return iter->second == v;
	}

	void IHeader::Add(const std::string& k, int v)
	{
		this->mHeader.emplace_back(k, std::to_string(v));
	}

	void IHeader::Add(const IHeader& header)
	{
		for(const std::pair<std::string, std::string> & iter : header.mHeader)
		{
			this->mHeader.emplace_back(iter);
		}
	}

	void IHeader::Add(const std::string& k, long long v)
	{
		this->mHeader.emplace_back(k, std::to_string(v));
	}

	void IHeader::WriteLua(lua_State* l, const tcp::IHeader& header)
	{
		Lua::UserDataParameter::UserDataStruct<const IHeader*>::Write(l, &header);
	}

	int IHeader::LuaGet(lua_State* L)
	{
		IHeader* head = Lua::UserDataParameter::Read<IHeader*>(L, 1);
		const char* key = luaL_checkstring(L, 2);
		if (head == nullptr || key == nullptr)
		{
			return 0;
		}
		std::string value;
		if (!head->Get(key, value))
		{
			return 0;
		}
		lua_pushlstring(L, value.c_str(), value.size());
		return 1;
	}

	int IHeader::LuaToString(lua_State* L)
	{
		IHeader* head = Lua::UserDataParameter::Read<IHeader*>(L, 1);
		if(head == nullptr)
		{
			return 0;
		}
		json::w::Document document;
		for(const auto & iter : head->GetValue())
		{
			document.Add(iter.first.c_str(), iter.second);
		}
		wrap::string<true> json;
		if(!document.Serialize(json))
		{
			return 0;
		}
		lua_pushlstring(L, json.c_str(), json.size());
		return 1;
	}

	void IHeader::Add(const std::string& k, const std::string& v)
	{
		if(k.empty() || v.empty())
		{
			return;
		}
		this->mHeader.emplace_back(k, v);
	}

	void IHeader::Set(const std::string& k, const std::string& v)
	{
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
				[&k](const std::pair<std::string, std::string>& item)
				{
					return item.first == k;
				});
		if (iter != this->mHeader.end())
		{
			iter->second = v;
			return;
		}
		this->mHeader.emplace_back(k, v);
	}

	bool IHeader::Del(const std::string& k)
	{
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
				[&k](const std::pair<std::string, std::string>& item)
				{
					return item.first == k;
				});
		if (iter == this->mHeader.end())
		{
			return false;
		}
		this->mHeader.erase(iter);
		return true;
	}

	bool IHeader::Del(const std::string& k, int& v)
	{
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
				[&k](const std::pair<std::string, std::string>& item)
				{
					return item.first == k;
				});
		if (iter == this->mHeader.end())
		{
			return false;
		}
		if (!help::Math::ToNumber(iter->second, v))
		{
			return false;
		}
		this->mHeader.erase(iter);
		return true;
	}

	bool IHeader::Del(const std::string& k, std::string& v)
	{
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
				[&k](const std::pair<std::string, std::string>& item)
				{
					return item.first == k;
				});
		if (iter == this->mHeader.end())
		{
			return false;
		}
		v = iter->second;
		return true;
	}
}
