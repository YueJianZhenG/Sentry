//
// Created by yy on 2023/11/19.
//

#include "Content.h"
#include <sstream>
#include "Yyjson/Lua/ljson.h"
#include "Util/Tools/Math.h"
#include "fmt.h"
#include "Url.h"
#include "Util/File/DirectoryHelper.h"
#include "Util/File/FileHelper.h"
#include "Lua/Engine/LuaInclude.h"
#include "Util/Tools/String.h"
#include "Util/Tools/Guid.h"

namespace http
{
	bool FromContent::OnDecode()
	{
		std::vector<std::string> result;
		help::Str::Split(this->mContent, '&', result);
		for (const std::string& filed: result)
		{
			size_t pos1 = filed.find('=');
			if (pos1 == std::string::npos)
			{
				return false;
			}
			std::string key = filed.substr(0, pos1);
			std::string val = filed.substr(pos1 + 1);
			if (!key.empty() && !val.empty())
			{
				if (val.find("%2F") != std::string::npos)
				{
					help::Str::Replace(val, "%2F", "/");
				}
				this->mParameters.emplace(key, val);
			}
		}
		return true;
	}

	std::string FromContent::ToStr() const
	{
		size_t index = 0;
		std::stringstream ss;
		auto iter = this->mParameters.begin();
		for (; iter != this->mParameters.end(); ++iter, index++)
		{
			const std::string& key = iter->second;
			if (key == http::query::Access || key == http::query::UserId)
			{
				continue;
			}
			ss << iter->first << "=" << iter->second;
			if (index != this->mParameters.size() - 1)
			{
				ss << "&";
			}
		}
		return ss.str();
	}

	void FromContent::WriteToLua(lua_State* lua)
	{
		lua_createtable(lua, 0, (int)this->mParameters.size());
		for (auto iter = this->mParameters.begin(); iter != this->mParameters.end(); ++iter)
		{
			const std::string& key = iter->first;
			const std::string& value = iter->second;
			lua_pushlstring(lua, value.c_str(), value.size());
			lua_setfield(lua, -2, key.c_str());
		}
	}

	bool FromContent::Add(const std::string& k, int v)
	{
		auto iter = this->mParameters.find(k);
		if (iter != this->mParameters.end())
		{
			return false;
		}
		this->mParameters.emplace(k, std::to_string(v));
		return true;
	}

	bool FromContent::Add(const std::string& k, const std::string& v)
	{
		if(k.empty() || v.empty())
		{
			return false;
		}
		auto iter = this->mParameters.find(k);
		if (iter != this->mParameters.end())
		{
			return false;
		}
		this->mParameters.emplace(k, v);
		return true;
	}

	void FromContent::Set(const std::string& k, int v)
	{
		this->mParameters[k] = std::to_string(v);
	}

	void FromContent::Set(const std::string& k, const std::string& v)
	{
		this->mParameters[k] = v;
	}

	bool FromContent::Get(std::vector<std::string>& keys) const
	{
		if (this->mParameters.empty())
		{
			return false;
		}
		keys.reserve(this->mParameters.size());
		auto iter = this->mParameters.begin();
		for (; iter != this->mParameters.end(); ++iter)
		{
			keys.emplace_back(iter->first);
		}
		return true;
	}

	bool FromContent::Get(const std::string& key, std::string& value) const
	{
		auto iter = this->mParameters.find(key);
		if (iter == this->mParameters.end())
		{
			return false;
		}
		value = iter->second;
		return true;
	}

	bool FromContent::Decode(const std::string& content)
	{
		this->mContent = content;
		return this->OnDecode();
	}

	bool FromContent::Get(const std::string& k, int& value) const
	{
		auto iter = this->mParameters.find(k);
		if (iter == this->mParameters.end())
		{
			return false;
		}
		const std::string& str = iter->second;
		return help::Math::ToNumber(str, value);
	}

	bool FromContent::Get(const std::string& k, long long& value) const
	{
		auto iter = this->mParameters.find(k);
		if (iter == this->mParameters.end())
		{
			return false;
		}
		const std::string& str = iter->second;
		return help::Math::ToNumber(str, value);
	}

	void FromContent::OnWriteHead(std::ostream& os)
	{
		std::string content = this->Serialize();
		this->mContent = http::url::encode(content);
		os << http::Header::ContentType << ": " << http::Header::FORM << "\r\n";
		os << http::Header::ContentLength << ": " << this->mContent.size() << "\r\n";
	}

	int FromContent::OnRecvMessage(std::istream& is, size_t size)
	{
		size_t count = 0;
		char buffer[256] = { 0};
		do
		{
			count = is.readsome(buffer, sizeof(buffer));
			if(count > 0)
			{
				this->mContent.append(buffer, count);
			}
		}
		while(count > 0);
		return tcp::read::some;
	}

	int FromContent::OnWriteBody(std::ostream& os)
	{
		os.write(this->mContent.c_str(), (std::streamsize)this->mContent.size());
		return 0;
	}


	std::string FromContent::Serialize() const
	{
		size_t index = 0;
		std::string result;
		auto iter = this->mParameters.begin();
		for (; iter != this->mParameters.end(); ++iter, index++)
		{
			if(!result.empty())
			{
				result += '&';
			}
			result.append(iter->first);
			result += '=';
			result.append(iter->second);
		}
		return result;
	}
}

namespace http
{
	int JsonContent::OnRecvMessage(std::istream& is, size_t size)
	{
		size_t count = 0;
		char buffer[512] = { 0 };
		do
		{
			count = is.readsome(buffer, sizeof(buffer));
			if (count > 0)
			{
				this->mJson.append(buffer, count);
			}
		} while (count > 0);
		return tcp::read::some;
	}

	void JsonContent::OnWriteHead(std::ostream& os)
	{
		os << http::Header::ContentType << ": " << http::Header::JSON << "\r\n";
		os << http::Header::ContentLength << ": " << this->mJson.size() << "\r\n";
	}

	void JsonContent::Write(const json::w::Document& document)
	{
		this->mJson.clear();
		document.Serialize(&this->mJson);
	}

	void JsonContent::WriteToLua(lua_State* lua)
	{
		lua::yyjson::write(lua, this->mDocument.GetValue());
	}

	int JsonContent::OnWriteBody(std::ostream& os)
	{
		os.write(this->mJson.c_str(), this->mJson.size());
		return 0;
	}
}

namespace http
{
	int XMLContent::OnRecvMessage(std::istream& is, size_t size)
	{
		size_t count = 0;
		char buffer[512] = { 0 };
		do
		{
			count = is.readsome(buffer, sizeof(buffer));
			if (count > 0)
			{
				this->mXml.append(buffer, count);
			}
		} while (count > 0);
		return tcp::read::some;
	}

	void XMLContent::OnWriteHead(std::ostream& os)
	{
		if (this->mXml.empty())
		{
			this->Encode(this->mXml);
		}
		os << http::Header::ContentType << ": " << http::Header::XML << "\r\n";
		os << http::Header::ContentLength << ": " << this->mXml.size() << "\r\n";
	}

	void XMLContent::WriteToLua(lua_State* lua)
	{

	}

	int XMLContent::OnWriteBody(std::ostream& os)
	{
		os.write(this->mXml.c_str(), this->mXml.size());
		return 0;
	}
}

namespace http
{
	void TextContent::OnWriteHead(std::ostream& os)
	{
		os << http::Header::ContentType << ": " << this->mConType << "\r\n";
		os << http::Header::ContentLength << ": " << this->mContent.size() << "\r\n";
	}

	void TextContent::Append(const std::string& content)
	{
		this->mContent.append(content);
	}

	int TextContent::OnWriteBody(std::ostream& os)
	{
		if (!this->mContent.empty())
		{
			//std::cout << this->mContent << std::endl;
			os.write(this->mContent.c_str(), this->mContent.size());
			return 0;
		}
		return 0;
	}

	void TextContent::WriteToLua(lua_State* lua)
	{
		lua_pushlstring(lua, this->mContent.c_str(), this->mContent.size());
	}

	int TextContent::OnRecvMessage(std::istream& is, size_t size)
	{
		char buffer[512] = { 0 };
		size_t count = is.readsome(buffer, sizeof(buffer));
		while (count > 0)
		{
			this->mContent.append(buffer, count);
			if (this->mMaxSize > 0 && this->mContent.size() >= this->mMaxSize)
			{
				return tcp::read::big_long;
			}
			count = is.readsome(buffer, sizeof(buffer));
		}
		return tcp::read::some;
	}

	void TextContent::SetContent(const std::string& type, const std::string& content)
	{
		this->mConType = type;
		this->mContent = content;
	}

	void TextContent::SetContent(const std::string& type, const char* content, size_t size)
	{
		this->mConType = type;
		this->mContent.assign(content, size);
	}
}

namespace http
{
	FileContent::FileContent()
	{
		this->mFileSize = 0;
		this->mSendSize = 0;
		this->mMaxSize = 0;
		this->mType = http::Header::Bin;
	}

	FileContent::FileContent(std::string t)
			: mType(std::move(t))
	{
		this->mFileSize = 0;
		this->mSendSize = 0;
		this->mMaxSize = 0;
	}

	FileContent::~FileContent()
	{
		if (this->mFile.is_open())
		{
			this->mFile.close();
		}
	}

	bool FileContent::OnDecode()
	{
		if (this->mFile.is_open())
		{
			this->mFile.close();
		}
		return true;
	}

	void FileContent::WriteToLua(lua_State* l)
	{
		lua_pushlstring(l, this->mPath.c_str(), this->mPath.size());
	}

	void FileContent::OnWriteHead(std::ostream& os)
	{
		if(!this->mType.empty())
		{
			os << http::Header::ContentType << ": " << this->mType << "\r\n";
		}
		os << http::Header::ContentLength << ": " << this->mFileSize << "\r\n";
	}

	int FileContent::OnRecvMessage(std::istream& is, size_t size)
	{
		size_t count = 0;
		char buffer[128] = { 0};
		do
		{
			count = is.readsome(buffer, sizeof(buffer));
			if(count > 0)
			{
				this->mFileSize += count;
				this->mFile.write(buffer, count);
			}
		}
		while(count > 0);
		this->mFile.flush();
		return tcp::read::some;
	}

	bool FileContent::MakeFile(const std::string& path)
	{
		std::string director;
		if (!help::dir::GetDirByPath(path, director))
		{
			return false;
		}
		if (!help::dir::DirectorIsExist(director))
		{
			help::dir::MakeDir(director);
		}
		this->mFile.open(path, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!this->mFile.is_open())
		{
			return false;
		}
		this->mPath = path;
		return true;
	}

	bool FileContent::OpenFile(const std::string& path)
	{
		this->mFile.open(path, std::ios::in | std::ios::binary);
		if (!this->mFile.is_open())
		{
			return false;
		}
		this->mPath = path;
		std::string suffix;
		help::fs::GetFileSize(path, this->mFileSize);
		if(this->mType.empty() && help::fs::GetFileType(path, suffix))
		{
			this->mType = http::GetContentType(suffix);
		}
		return true;
	}

	bool FileContent::OpenFile(const std::string& path, const std::string& type)
	{
		this->mType = type;
		this->mPath = path;
		return this->OpenFile(path);
	}

	int FileContent::OnWriteBody(std::ostream& os)
	{
		char buff[512] = { 0 };
		size_t size = this->mFile.read(buff, sizeof(buff)).gcount();
		if (size > 0)
		{
			os.write(buff, size);
			this->mSendSize += size;
			//std::cout << this->mSendSize << ":" << this->mFileSize << " => " << this->mFileSize - this->mSendSize << std::endl;
			return this->mFileSize - this->mSendSize;
		}
		return 0;
	}
}

namespace http
{

	void ChunkedContent::WriteToLua(lua_State* l)
	{
		lua_pushlstring(l, this->mContent.c_str(), this->mContent.size());
	}

	bool ChunkedContent::OnDecode()
	{
		return true;
	}

	int ChunkedContent::OnRecvMessage(std::istream& buffer, size_t size)
	{
		this->mIndex++;
		if(this->mIndex % 2 != 0)
		{
			std::string line;
			std::getline(buffer, line);
			if(line.back() == '\r')
			{
				line.pop_back();
			}
			if(line.empty())
			{
				return tcp::read::done;
			}
			this->mCount = std::stoul(line, nullptr, 16);
			return (int)this->mCount + 2;
		}
		std::unique_ptr<char[]> buff = std::make_unique<char[]>(size);
		{
			size_t count = buffer.readsome(buff.get(), size);
			this->mContent.append(buff.get(), count);
		}
		if(size < this->mCount)
		{
			this->mIndex--;
			this->mCount -= size;
			return this->mCount;
		}
		return tcp::read::line;
	}

	void ChunkedContent::OnWriteHead(std::ostream& os)
	{
		os << http::Header::TransferEncoding << ": chunked" << http::CRLF;
		os << http::Header::ContentType << fmt::format(": {}; charset=utf-8\r\n", this->mContType);
	}

	void ChunkedContent::SetContent(const std::string& content)
	{
		this->mContent = content;
		this->mContType = http::Header::TEXT;
	}

	void ChunkedContent::SetContent(const std::string& type, const std::string& content)
	{
		this->mContType = type;
		this->mContent = content;
	}

	int ChunkedContent::OnWriteBody(std::ostream& os)
	{
		size_t size = this->mContent.size();
		if (size > 0)
		{
			os << std::hex << size << "\r\n";
			os.write(this->mContent.c_str(), size) << "\r\n";
		}
		os << "\r\n";
		return 0;
	}
}

namespace http
{
	MultipartFromContent::MultipartFromContent()
			: mDone(false), mMaxCount(1024 * 1024 * 5), mReadCount(0)
	{
		this->mLength = 0;
	}

	bool MultipartFromContent::OnDecode()
	{
		this->mDone = true;
		this->mFile.close();
		return true;
	}

	bool MultipartFromContent::Add(const std::string& k, const std::string& v)
	{
		if (!help::fs::FileIsExist(v.c_str()))
		{
			auto iter = this->mFromData.find(k);
			if (iter != this->mFromData.end())
			{
				return false;
			}
			std::stringstream ss;
			ss << http::Header::ContentDisposition << ": form-data; ";
			ss << fmt::format("name=\"{}\"", k) << "\r\n\r\n" << v << "\r\n";
			this->mHeader.emplace_back(ss.str());
			return true;
		}
		this->mFile.open(v, std::ios::in | std::ios::binary);
		if (!this->mFile.is_open())
		{
			return false;
		}
		this->mLength = 0;
		std::string fileName, fileType;
		if (!help::Str::GetFileName(v, fileName) && help::fs::GetFileType(v, fileType))
		{
			return false;
		}
		this->mPath = v;
		std::stringstream ss;
		std::string contenType(http::GetContentType(fileType));
		ss << http::Header::ContentDisposition << ": form-data; ";
		ss << fmt::format(R"(name="{}"; filename="{}")", k, fileName) << "\r\n";
		ss << http::Header::ContentType << ": " << contenType << "\r\n\r\n";

		this->mHeader.emplace_back(ss.str());
		return true;
	}

	size_t MultipartFromContent::GetContentLength() const
	{
		size_t size = 0;
		size_t length = 0;
		if (help::fs::GetFileSize(this->mPath, size))
		{
			length += size;
		}
		for (const std::string & str : this->mHeader)
		{
			length += str.size();
			length += (this->mBoundary.size() + 4);
		}
		length += (this->mBoundary.size() + 8);
		return length;
	}

	int MultipartFromContent::OnWriteBody(std::ostream& os)
	{
		for (const std::string& value: this->mHeader)
		{
			os << "--" << this->mBoundary << "\r\n";
			os.write(value.c_str(), value.size());
		}

		char buff[1024] = { 0 };
		while (!this->mFile.eof())
		{
			this->mFile.read(buff, sizeof(buff));
			size_t len = this->mFile.gcount();
			if (len > 0)
			{
				os.write(buff, len);
			}
		}
		os << "\r\n--" << this->mBoundary << "--\r\n";
		/*
				std::stringstream ss;
				ss << os.rdbuf();
				std::string str = ss.str();
				help::fs::WriterFile("./a.json", str);
			*/
		return 0;
	}

	void MultipartFromContent::OnWriteHead(std::ostream& os)
	{
		this->mBoundary = fmt::format("----{}", help::ID::Gen());
		os << http::Header::ContentType << ": " << "multipart/form-data; boundary=" << this->mBoundary << "\r\n";
		os << http::Header::ContentLength << ": " << this->GetContentLength() << "\r\n";

	}

	void MultipartFromContent::WriteToLua(lua_State* l)
	{
		lua_pushlstring(l, this->mPath.c_str(), this->mPath.size());
	}

	void MultipartFromContent::Init(const std::string& dir, size_t limit)
	{
		this->mDir = dir;
		this->mReadCount = 0;
		this->mMaxCount = limit;
	}

	std::string MultipartFromContent::ToStr() const
	{
		json::w::Document document;
		document.Add("path", this->mPath);
		document.Add("name", this->mFileName);
		document.Add("size", this->mReadCount);
		document.Add("type", this->mContType);
		return document.JsonString();
	}

	int MultipartFromContent::OnRecvMessage(std::istream& buffer, size_t size)
	{
		this->mReadCount += size;
		if (this->mMaxCount > 0 && this->mReadCount >= this->mMaxCount)
		{
			return tcp::read::decode_error;
		}
		if (this->mBoundary.empty())
		{
			if (!std::getline(buffer, this->mBoundary))
			{
				return tcp::read::line;
			}
			this->mReadCount += this->mBoundary.size() + 1;
			if (this->mBoundary.back() == '\r')
			{
				this->mBoundary.pop_back();
			}
			return tcp::read::line;
		}

		if (this->mFile.is_open())
		{
			std::unique_ptr<char[]> buff = std::make_unique<char[]>(size);
			size_t count = buffer.readsome(buff.get(), size);
			if (count > 0)
			{
				if (strstr(buff.get(), this->mBoundary.c_str()) != nullptr)
				{
					this->mFile.close();
					return tcp::read::line;
				}
				this->mFile.write(buff.get(), count);
				this->mFile.flush();
			}
			return tcp::read::line;
		}

		std::string line;
		if (std::getline(buffer, line))
		{
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}

			if (line == this->mBoundary)
			{
				return tcp::read::line;
			}
			else if (line.empty())
			{
				return tcp::read::line;
			}
			else if (line.find("Content-Disposition:") != std::string::npos)
			{
				size_t name_pos = line.find("name=\"");
				if (name_pos != std::string::npos)
				{
					size_t name_end = line.find('\"', name_pos + 6);
					this->mFieldName = line.substr(name_pos + 6, name_end - name_pos - 6);
				}

				size_t filename_pos = line.find("filename=\"");
				if (filename_pos != std::string::npos)
				{
					size_t filename_end = line.find('\"', filename_pos + 10);
					this->mFileName = line.substr(filename_pos + 10, filename_end - filename_pos - 10);
				}
				return tcp::read::line;
			}

			if (!this->mFileName.empty())
			{
				std::string director;
				this->mFromData.emplace(this->mFieldName, this->mFileName);
				this->mPath = fmt::format("{}/{}", this->mDir, this->mFileName);
				if (help::dir::GetDirByPath(this->mPath, director))
				{
					help::dir::MakeDir(director);
				}
				this->mFile.open(this->mPath, std::ios::out | std::ios::binary);
				if (!this->mFile.is_open())
				{
					return tcp::read::decode_error;
				}
				std::getline(buffer, line);
				//this->mFileName.clear();
			}
			else if (!this->mFieldName.empty())
			{
				if (!line.empty() && line.back() == '\r')
				{
					line.pop_back();
				}
				if (!line.empty())
				{
					this->mFromData.emplace(this->mFieldName, line);
				}
				//std::cout << this->mFieldName << ": " << line << std::endl;
			}
		}
		return tcp::read::line;
	}

	BinContent::BinContent()
	{

	}

	void BinContent::OnWriteHead(std::ostream& os)
	{
		os << http::Header::ContentType << ": " << http::Header::PB << "\r\n";
		os << http::Header::ContentLength << ": " << this->mBody.size() << "\r\n";
	}

	int BinContent::OnWriteBody(std::ostream& os)
	{
		os.write(this->mBody.c_str(), this->mBody.size());
		return 0;
	}

	void BinContent::WriteToLua(lua_State* l)
	{
		lua_pushlstring(l, this->mBody.c_str(), this->mBody.size());
	}

	int BinContent::OnRecvMessage(std::istream& is, size_t size)
	{
		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
		size_t count = is.readsome(buffer.get(), size);
		if (count > 0)
		{
			this->mBody.append(buffer.get(), count);
		}
		return tcp::read::some;
	}

}

namespace http
{
	int StringContent::OnWriteBody(std::ostream& os)
	{
		os.write(this->mBody.c_str(), this->mBody.size());
		return 0;
	}

	void StringContent::WriteToLua(lua_State* L)
	{
		lua_pushlstring(L, this->mBody.c_str(), this->mBody.size());
	}

	int StringContent::OnRecvMessage(std::istream& is, size_t size)
	{
		return 0;
	}


}