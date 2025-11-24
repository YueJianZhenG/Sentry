//
// Created by yjz on 2022/10/27.
//
#include"HttpRequest.h"
#include"Util/Tools/TimeHelper.h"
#include"Log/Common/CommonLogDef.h"
#include"Yyjson/Document/Document.h"
#include"Lua/Engine/UserDataParameter.h"

namespace http
{
    Request::Request() : mSockId(0)
    {
		this->mConeSize = 0;
		this->mTimeout = 0;
		this->mDecodeStatus = tcp::Decode::None;
    }

	Request::Request(const char* method)
		: mSockId(0), mUrl(method)
	{
		this->mConeSize = 0;
		this->mTimeout = 0;
		this->mDecodeStatus = tcp::Decode::None;
	}

	bool Request::GetIp(std::string& ip) const
	{
		if(this->mHead.Get(http::Header::ProxyIP, ip))
		{
			return true;
		}
		return this->mHead.Get(http::Header::RealIp, ip);
	}

    bool Request::SetUrl(const std::string &url)
	{
		return this->mUrl.Decode(url);
	}

	bool Request::SetUrl(const std::string& url, const http::FromContent& fromData)
	{
		std::string query = fromData.Serialize();
		return this->mUrl.Decode(fmt::format("{}?{}", url, query));
	}

    int Request::OnRecvMessage(std::istream& buffer, size_t size)
	{
		if(this->mDecodeStatus == tcp::Decode::None)
		{
			int code = this->mUrl.OnRecvMessage(buffer, size);
			if(code != tcp::read::done)
			{
				return code;
			}
			this->mDecodeStatus = tcp::Decode::MessageHead;
			return tcp::read::line;
		}
		if(this->mDecodeStatus == tcp::Decode::MessageHead)
		{
			int code = this->mHead.OnRecvMessage(buffer, size);
			if(code != tcp::read::done)
			{
				return code;
			}
			this->mConeSize = 0;
			std::string contentType;
			this->mDecodeStatus = tcp::Decode::MessageBody;
			if(this->mHead.GetContentLength(this->mConeSize))
			{
				if(this->mConeSize == 0)
				{
					return tcp::read::decode_error;
				}
				if(this->mBody != nullptr)
				{
					this->mBody->SetContentLength(this->mConeSize);
				}
			}
			return tcp::read::pause;
		}
		if(this->mDecodeStatus == tcp::Decode::MessageBody)
		{
			if(size == 0)
			{
				if(this->mBody != nullptr && !this->mBody->OnDecode())
				{
					LOG_ERROR("parse http body fail : {}", this->mUrl.ToString());
					return tcp::read::decode_error;
				}
				return tcp::read::done;
			}
			int flag = this->mBody->OnRecvMessage(buffer, size);		
			if(this->mConeSize > 0 && this->mBody->ContentLength() >= this->mConeSize)
			{
				flag = tcp::read::done;
			}

			if (flag == tcp::read::content_length)
			{
				if (this->mConeSize <= 0)
				{
					return tcp::read::decode_error;
				}
				return this->mConeSize;
			}
			if(flag == 0 && !this->mBody->OnDecode())
			{
				return tcp::read::decode_error;
			}
			return flag;
		}
		return tcp::read::done;
	}

	void Request::Clear()
	{
		this->mUrl.Clear();
		this->mHead.Clear();
		this->mTimeout = 0;
		this->mBody.reset();
		this->mDecodeStatus = tcp::Decode::None;
	}

    int Request::OnSendMessage(std::ostream &os)
    {
		//this->mHead.SetKeepAlive(true);
		if(this->mDecodeStatus == tcp::Decode::None)
		{
			this->mUrl.OnSendMessage(os);
			this->mHead.Add("Host", this->mUrl.Host());
			this->mDecodeStatus = tcp::Decode::MessageHead;
		}
		if(this->mDecodeStatus == tcp::Decode::MessageHead)
		{
			if(this->mBody != nullptr)
			{
				this->mBody->OnWriteHead(os);
			}
			this->mHead.OnSendMessage(os);
			this->mDecodeStatus = tcp::Decode::MessageBody;
		}
		if(this->mBody && this->mDecodeStatus == tcp::Decode::MessageBody)
		{
			return this->mBody->OnWriteBody(os);
		}
		return 0;
    }

	int Request::WriteToLua(lua_State* lua) const
	{
		lua_createtable(lua, 0, 4);
		{
			lua_pushstring(lua, "head");
			tcp::IHeader::WriteLua(lua, this->mHead);
			lua_rawset(lua, -3);
		}
		{
			lua_pushstring(lua, "url");
			const std::string & url = this->mUrl.ToStr();
			lua_pushlstring(lua, url.c_str(), url.size());
			lua_rawset(lua, -3);

		}
		{
			const http::FromContent& fromData = this->mUrl.GetQuery();
			if(fromData.Size() > 0)
			{
				lua_pushstring(lua, "query");
				(const_cast<http::FromContent&>(fromData)).WriteToLua(lua);
				lua_rawset(lua, -3);
			}
			long long playerId = 0;
			if(fromData.Get(http::query::UserId, playerId))
			{
				lua_pushstring(lua, "userId");
				lua_pushinteger(lua, playerId);
				lua_rawset(lua, -3);
			}
		}
		if(this->mBody != nullptr)
		{
			lua_pushstring(lua, "data");
			this->mBody->WriteToLua(lua);
			lua_rawset(lua, -3);
		}
		return 1;
	}

	std::string Request::ToString()
	{
		std::string str;
		json::w::Document jsonWriter;
		jsonWriter.Add("url", this->mUrl.ToStr());
		jsonWriter.Add("method", this->mUrl.Method());
		jsonWriter.Add("version", this->mUrl.Version());
    	std::unique_ptr<json::w::Value> headObject = jsonWriter.AddObject("head");

    	for(const std::pair<std::string, std::string> & iter : this->mHead.GetValue())
    	{
    		headObject->Add(iter.first.c_str(), iter.second);
    	}

		if(this->mUrl.GetQuery().Size() > 0)
		{
			jsonWriter.Add("data", this->mUrl.Query()->ToStr());
		}
		else if(this->mBody != nullptr)
		{
			jsonWriter.Add("data", this->mBody->ToStr());
		}
		std::string json;
		jsonWriter.Serialize(&json, true);
		return json;
	}

	void Request::WriteMessageToLua(lua_State* lua) const
	{
		if(this->mBody == nullptr)
		{
			lua_pushnil(lua);
			return;
		}
		this->mBody->WriteToLua(lua);
	}

	bool Request::IsMethod(const std::string& method) const
	{
		if(method.empty()) return true;
		const std::string & m = this->mUrl.Method();
		return m.find(method) != std::string::npos;
	}

	void Request::SetContent(const std::string & t, const std::string& content)
	{
		this->SetContent(t,  content.c_str(), content.size());
	}

	void Request::SetContent(const json::w::Document& document)
	{
		std::unique_ptr<http::JsonContent> jsonContent = std::make_unique<http::JsonContent>();
		{
			jsonContent->Write(document);
			this->mBody = std::move(jsonContent);
		}
	}

	void Request::SetContent(const std::string & t, const char* content, size_t size)
	{
		std::unique_ptr<http::TextContent> customData(new http::TextContent());
		{
			customData->SetContent(t, content, size);
			this->mBody = std::move(customData);
		}
	}
}