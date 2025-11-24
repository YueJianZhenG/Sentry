//
// Created by yy on 2023/12/10.
//

#include"Url.h"
#include<regex>
#include <iomanip>
#include <bundled/format.h>

#include"sstream"

namespace http
{
	inline int hexToDec(char c) {
		if (isdigit(static_cast<unsigned char>(c))) {
			return c - '0';
		} else if (isupper(static_cast<unsigned char>(c))) {
			return c - 'A' + 10;
		} else if (islower(static_cast<unsigned char>(c))) {
			return c - 'a' + 10;
		}
		return -1;
	}

	std::string url::decode(const std::string& url)
	{
		std::ostringstream decoded;
		for (size_t i = 0; i < url.size(); ++i)
		{
			if (url[i] == '%' && i + 2 < url.size())
			{
				std::istringstream hex(url.substr(i + 1, 2));
				int value;
				if (hex >> std::hex >> value)
				{
					decoded << static_cast<char>(value);
					i += 2;
				}
			}
			else if (url[i] == '+')
			{
				decoded << ' ';
			}
			else
			{
				decoded << url[i];
			}
		}
		return decoded.str();
	}

	std::string url::encode(const std::string& str)
	{
		std::ostringstream encoded;
		encoded << std::hex << std::uppercase;
		for (char c: str)
		{
			if (std::isalnum(static_cast<unsigned char>(c)) ||
				c == '-' || c == '_' || c == '.' || c == '~' || c == '&' || c == '=')
			{
				encoded << c;
			}
			else if (c == ' ')
			{
				encoded << "+";
			}
			else
			{
				unsigned char uc = static_cast<unsigned char>(c);
				encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(uc);
			}
		}

		return encoded.str();
	}

	Url::Url() : mVersion(http::Version), mReadCount(0)
	{
	}

	Url::Url(const char* method)
			: mVersion(http::Version),
			  mMethod(method), mReadCount(0)
	{

	}

	int Url::OnSendMessage(std::ostream& os)
	{
		if (this->mPath.empty())
		{
			this->mPath = "/";
		}
		os << this->mMethod << " " << this->mPath
		   << " " << this->mVersion << http::CRLF;
		return 0;
	}

	void Url::Clear()
	{
		this->mReadCount = 0;
		this->mUrl.clear();
		this->mPort.clear();
		this->mHost.clear();
		this->mPath.clear();
		this->mQuery.Clear();
		this->mProto.clear();
		this->mMethod.clear();
		this->mVersion = http::Version;
	}

	int Url::OnRecvMessage(std::istream& os, size_t size)
	{
		this->mReadCount++;
		if (this->mReadCount >= 3 || size <= 2)
		{
			return tcp::read::decode_error;
		}

		if (!std::getline(os, this->mMethod, ' '))
		{
			return tcp::read::decode_error;
		}
		std::string url;
		if (!std::getline(os, url, ' '))
		{
			return tcp::read::decode_error;
		}
		if (!std::getline(os, this->mVersion))
		{
			return tcp::read::decode_error;
		}
		if (this->mVersion.back() == '\r')
		{
			this->mVersion.pop_back();
		}

		if (this->mMethod.empty() || url.empty() || this->mVersion.empty())
		{
			return tcp::read::decode_error;
		}
		this->mUrl = http::url::decode(url);
		size_t pos = this->mUrl.find('?');
		if (pos == std::string::npos)
		{
			this->mPath = this->mUrl;
			return tcp::read::done;
		}
		this->mPath = this->mUrl.substr(0, pos);
		std::string query = this->mUrl.substr(pos + 1);
		return this->mQuery.Decode(query) ? tcp::read::done : tcp::read::decode_error;
	}


	bool Url::Decode(const std::string& url)
	{
		std::cmatch what;
		std::regex pattern("(http|https)://([^/ :]+):?([^/ ]*)(/.*)?");
		if (std::regex_match(url.c_str(), what, pattern))
		{
			this->mHost = std::string(what[2].first, what[2].second);
			this->mPath = std::string(what[4].first, what[4].second);
			this->mProto.append(what[1].first, what[1].second);
			this->mPort = std::string(what[3].first, what[3].second);
			if (this->mPath.empty())
			{
				this->mPath = "/";
			}
			else
			{
				size_t pos = this->mPath.find('?');
				if(pos != std::string::npos)
				{
					std::string path = this->mPath.substr(0, pos);
					std::string param = this->mPath.substr(pos + 1);
					this->mPath = fmt::format("{}?{}", path,  http::url::encode(param));
				}
			}
			if (this->mPort.empty())
			{
				this->mPort = this->IsHttps() ? "443" : "80";
			}
			this->mUrl = url;
			return !this->mHost.empty();
		}
		return false;
	}
}
