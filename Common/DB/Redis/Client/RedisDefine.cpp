//
// Created by zmhy0073 on 2021/12/2.
//

#include"RedisDefine.h"
#include"Async/Lua/LuaCoroutine.h"
#include"Yyjson/Document/Document.h"
namespace redis
{
	constexpr const char * CRLF = "\r\n";
    int Request::OnSendMessage(std::ostream& readStream)
    {
        if(this->mParameters.empty())
        {
            readStream << this->mCommand << CRLF;
            return 0;
        }
        readStream << "*" << this->mParameters.size() + 1 << CRLF;
        readStream << "$" << this->mCommand.size() << CRLF << this->mCommand << CRLF;
        for(const std::string & parameter : this->mParameters)
        {
			readStream << '$' << parameter.size() << CRLF << parameter << CRLF;
		}
		return 0;
    }

	void Request::Clear()
	{
		this->mCommand.clear();
		this->mParameters.clear();
	}

	std::string Request::ToString()
	{
		json::w::Document document;
		std::unique_ptr<json::w::Value> list = document.AddArray(this->mCommand.c_str());
		{
			for(const std::string & str : this->mParameters)
			{
				list->Push(str.c_str(), str.size());
			}
		}
		return document.JsonString();
	}

	std::unique_ptr<Request> Request::MakeLua(const std::string& key, const std::string& func, const std::string & json)
	{
		std::unique_ptr<Request> request = std::make_unique<Request>();
		{
			request->SetCommand("EVALSHA");
			Request::InitParameter(request.get(), key, 1, func, json);
		}
		return request;
	}
}

namespace redis
{
    Response::Response()
    {
		this->element.type = 0;
    }

	std::string Response::ToString()
	{
		switch(this->element.type)
		{
			case type::String:
			case type::Number:
			case type::BinString:
				return this->element.message;
			case type::Array:
			{
				json::w::Document document(true);
				for(const Element & val : this->element.list)
				{
					if(val.type == redis::type::Number)
					{
						document.Push(val.number);
						continue;
					}
					document.Push(val.message);
				}
				return document.JsonString();
			}
		}
		return this->element.message;
	}


	void Response::Clear()
	{
		this->element.type = 0;
		this->element.number = 0;
		this->element.list.clear();
	}

	int Response::WriteToLua(lua_State* lua) const
	{
		return this->WriteElementLua(this->element, lua);
	}

	int Response::WriteElementLua(const redis::Element& element, lua_State* lua) const
	{
		switch (element.type)
		{
			case redis::type::Number:
				lua_pushinteger(lua, element.number);
				return 1;
			case redis::type::String:
			case redis::type::BinString:
			{
				const std::string& str = element.message;
				lua_pushlstring(lua, str.c_str(), str.size());
				return 1;
			}
			case redis::type::Array:
			{
				int index = 1;
				lua_createtable(lua, 0, (int)element.list.size());
				for(const Element& item : element.list)
				{
					this->WriteElementLua(item, lua);
					lua_seti(lua, -2, index++);
				}
				return 1;
			}
			default:
				return 0;
		}
		return 1;
	}
}

namespace acs
{
    RedisTask::RedisTask(int ms)
        : IRpcTask<redis::Response>(ms)
    {
		this->mMessage = nullptr;
    }

    LuaRedisTask::LuaRedisTask(lua_State *lua, int id)
        : IRpcTask<redis::Response>(id), mLua(lua)
    {
        this->mRef = 0;
        if(lua_isthread(this->mLua, -1))
        {
            this->mRef = luaL_ref(lua, LUA_REGISTRYINDEX);
        }
    }
    LuaRedisTask::~LuaRedisTask()
    {
        if(this->mRef != 0)
        {
            luaL_unref(this->mLua, LUA_REGISTRYINDEX, this->mRef);
        }
    }

    int LuaRedisTask::Await() noexcept
    {
        if(this->mRef == 0)
        {
            LOG_ERROR("not lua coroutine context yield failure");
            return 0;
        }
        return lua_yield(this->mLua, 0);
    }

    void LuaRedisTask::OnResponse(std::unique_ptr<redis::Response>& response) noexcept
	{
		int count = 0;
		if (response != nullptr)
		{
			count = response->WriteToLua(this->mLua);
		}
		Lua::Coroutine::Resume(this->mLua, count);
	}

	void LuaRedisTask::OnTimeout()
	{
		std::unique_ptr<redis::Response> response = std::make_unique<redis::Response>();
		{
			response->element.message = "time out";
			response->element.type = redis::type::Error;
		}
		response->WriteToLua(this->mLua);
		Lua::Coroutine::Resume(this->mLua, 1);
	}
}
