//
// Created by yjz on 2022/10/27.
//

#include"HttpResponse.h"
#include"Util/File/DirectoryHelper.h"
#include"Lua/Engine/UserDataParameter.h"
#include "Util/Tools/Math.h"
#include "Util/Tools/String.h"

namespace http
{
	Response::Response(HttpStatus code)
		: mCode((int)code)
	{
		this->mContSize = 0;
		this->mBody = nullptr;
		this->mVersion = http::Version;
		this->mCode = (int)HttpStatus::OK;
		this->mParseState = tcp::Decode::None;
	}

	int Response::OnRecvMessage(std::istream& buffer, size_t size)
	{
		if (size == 0)
		{
			if(this->mBody!= nullptr)
			{
				this->mBody->OnDecode();
			}
			return tcp::read::done;
		}
		if (this->mParseState == tcp::Decode::None)
		{
			buffer >> this->mVersion >> this->mCode;
			std::getline(buffer, this->mError);
			if (!this->mError.empty() && this->mError.back() == '\r')
			{
				this->mError.pop_back();
			}
			this->mParseState = tcp::Decode::MessageHead;
		}
		if (this->mParseState == tcp::Decode::MessageHead && !buffer.eof())
		{
			switch (this->mHead.OnRecvMessage(buffer, size))
			{
				case tcp::read::line:
					return tcp::read::line;
				case tcp::read::error:
					return tcp::read::error;
				case tcp::read::done:
					this->mParseState = tcp::Decode::MessageBody;
					break;
			}
//			if(this->mCode != (int)HttpStatus::OK)
//			{
//				return tcp::ReadDone;
//			}

			this->mContSize = 0;
			if(!this->mHead.GetContentLength(this->mContSize))
			{
				std::string chunked("chunked");
				std::string encodeding(http::Header::TransferEncoding);
				help::Str::Tolower(encodeding);
				if(!this->mHead.IsEqual(encodeding, chunked))
				{
					return tcp::read::done;
				}
				if(this->mBody == nullptr)
				{
					this->mBody = std::make_unique<http::ChunkedContent>();
				}
			}
			else if(this->mContSize == 0)
			{
				return tcp::read::done;
			}

			if(this->mBody == nullptr)
			{
				std::string content_type;
				this->mHead.GetContentType(content_type);
				if(content_type.find(http::Header::JSON) != std::string::npos)
				{
					this->mBody = std::make_unique<http::JsonContent>();
				}
				else if (content_type.find(http::Header::XML) != std::string::npos)
				{
					this->mBody = std::make_unique<http::XMLContent>();
				}
				else
				{
					this->mBody = std::make_unique<http::TextContent>();
				}
			}

		}
		if (this->mParseState == tcp::Decode::MessageBody)
		{
			int flag = this->mBody->OnRecvMessage(buffer, size);
			if(this->mContSize > 0 && this->mBody->ContentLength() >= this->mContSize)
			{
				flag = tcp::read::done;
			}
			return flag;
		}
		return tcp::read::error;
	}

	void Response::Json(const std::string& json)
	{
		this->Json(json.c_str(), json.size());
	}

	void Response::Json(const char* json, size_t size)
	{
		this->mBody = std::make_unique<http::JsonContent>(json, size);
	}

	void Response::Text(const char* text, size_t size)
	{
		std::unique_ptr<http::TextContent> custom = std::make_unique<http::TextContent>();
		{
			custom->SetContent(http::Header::TEXT, text, size);
		}
		this->mBody = std::move(custom);
	}

	void Response::SetContent(json::w::Document& document)
	{
		std::unique_ptr<http::JsonContent> jsonData = std::make_unique<http::JsonContent>();
		{
			jsonData->Write(document);
		}
		this->mBody = std::move(jsonData);
	}

	void Response::SetContent(const std::string & type, const std::string& str)
	{
		std::unique_ptr<http::TextContent> customData = std::make_unique<http::TextContent>();
		{
			customData->SetContent(type, str.c_str(), str.size());
		}
		this->mBody = std::move(customData);
	}

	bool Response::File(const std::string & type, const std::string& path)
	{
		this->mCode = (int)HttpStatus::OK;
		std::unique_ptr<http::FileContent> fileData(new http::FileContent());
		{
			if(!fileData->OpenFile(path, type))
			{
                this->mBody = nullptr;
                this->SetCode(HttpStatus::NOT_FOUND);
				return false;
			}
			this->mBody = std::move(fileData);
		}
		return true;
	}

	bool Response::OpenOrCreateFile(const std::string& path)
	{
		if(!help::dir::IsValidPath(path))
		{
			return false;
		}
		std::string dir;
		help::dir::GetDirByPath(path, dir);
		if(!help::dir::DirectorIsExist(dir))
		{
			help::dir::MakeDir(dir);
		}
		std::unique_ptr<http::FileContent> fileData = std::make_unique<http::FileContent>();
		{
			if(!fileData->MakeFile(path))
			{
				return false;
			}
			this->mBody = std::move(fileData);
		}
		return true;
	}

	void Response::Clear()
	{
		this->mHead.Clear();
		this->mError.clear();
		this->mVersion.clear();
		this->mBody.reset();
		this->mCode = (int)HttpStatus::OK;
		this->mParseState = tcp::Decode::None;
	}

	void Response::SetCode(HttpStatus code)
	{
		this->mCode = (int)code;
		this->mError = HttpStatusToString(code);
	}

	std::string Response::ToString()
	{
		std::string json;
		json::w::Document js;
		js.Add("status", this->mCode);
		js.Add("status_text", this->mError);
		js.AddObject("header", this->mHead.ToString());
		if(this->mBody != nullptr)
		{
			js.Add("data", this->mBody->ToStr());
		}
		js.Serialize(&json, true);
		return json;
	}

	int Response::WriteToLua(lua_State* lua) const
	{
		lua_createtable(lua, 0, 4);
		{
			lua_pushstring(lua, "code");
			lua_pushinteger(lua, this->mCode);
			lua_rawset(lua, -3);
		}
		{
			lua_pushstring(lua, "status");
			lua_pushstring(lua, this->mError.c_str());
			lua_rawset(lua, -3);
		}
		{
			lua_pushstring(lua, "head");
			tcp::IHeader::WriteLua(lua, this->mHead);
			lua_rawset(lua, -3);
		}
		if (this->mBody == nullptr)
		{
			return 1;
		}
		lua_pushstring(lua, "body");
		this->mBody->WriteToLua(lua);
		lua_rawset(lua, -3);
		return 1;
	}

	int Response::OnSendMessage(std::ostream& buffer)
	{
		HttpStatus code = (HttpStatus)this->mCode;
		if(this->mParseState == tcp::Decode::None)
		{
			buffer << http::Version << ' ' << this->mCode << ' '
				   << HttpStatusToString(code) << "\r\n";
			this->mParseState = tcp::Decode::MessageHead;
		}
		if(this->mParseState == tcp::Decode::MessageHead)
		{
			if(this->mBody != nullptr)
			{
				this->mBody->OnWriteHead(buffer);
			}
			this->mHead.OnSendMessage(buffer);
			this->mParseState = tcp::Decode::MessageBody;
		}
		if(this->mBody != nullptr)
		{
			return this->mBody->OnWriteBody(buffer);
		}
		return 0;
	}
}
