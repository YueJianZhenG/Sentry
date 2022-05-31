//
// Created by zmhy0073 on 2022/1/19.
//

#include"HttpAsyncRequest.h"
#include<regex>
#include"Http.h"
#include<iostream>
#include"Util/StringHelper.h"
namespace Sentry
{
	HttpAsyncRequest::HttpAsyncRequest(const std::string& method)
		: mMethod(method)
	{

	}

    bool HttpAsyncRequest::ParseUrl(const std::string &url)
    {
        std::cmatch what;
        std::string protocol;
        std::regex pattern("(http|https)://([^/ :]+):?([^/ ]*)(/.*)?");
        if (std::regex_match(url.c_str(), what, pattern)) {
            this->mHost = std::string(what[2].first, what[2].second);
            this->mPath = std::string(what[4].first, what[4].second);
            protocol = std::string(what[1].first, what[1].second);
            this->mPort = std::string(what[3].first, what[3].second);

            if (0 == this->mPort.length()) {
                this->mPort = "http" == protocol ? "80" : "443";
            }
            return true;
        }
        return false;
    }

	bool HttpAsyncRequest::AddHead(const std::string& key, int value)
	{
		auto iter = this->mHeadMap.find(key);
		if(iter != this->mHeadMap.end())
		{
			return false;
		}
		this->mHeadMap.emplace(key, std::to_string(value));
		return true;
	}

	bool HttpAsyncRequest::AddHead(const std::string& key, const std::string& value)
	{
		auto iter = this->mHeadMap.find(key);
		if(iter != this->mHeadMap.end())
		{
			return false;
		}
		this->mHeadMap.emplace(key, value);
		return true;
	}

	bool HttpAsyncRequest::Serailize(std::ostream& os)
	{
		os << this->mMethod << " " << this->mPath << " " << HttpVersion << "\r\n";
		os << "Host:" << this->mHost << "\r\n";
		for(auto iter = this->mHeadMap.begin(); iter != this->mHeadMap.end(); iter++)
		{
			const std::string & key = iter->first;
			const std::string & value = iter->second;
			os << key << ':' << value << "\r\n";
		}
		os << "Connection: close\r\n\r\n";
		this->WriteBody(os);
		return true;
	}
}


namespace Sentry
{
	HttpGetRequest::HttpGetRequest()
		: HttpAsyncRequest("GET")
	{

	}

	void HttpGetRequest::WriteBody(std::ostream& os)
	{

	}

	std::shared_ptr<HttpGetRequest> HttpGetRequest::Create(const std::string& url)
	{
		std::shared_ptr<HttpGetRequest> request(new HttpGetRequest());
		return request->ParseUrl(url) ? request : nullptr;
	}
}

namespace Sentry
{
	HttpPostRequest::HttpPostRequest()
		: HttpAsyncRequest("POST")
	{

	}

	void HttpPostRequest::AddBody(const std::string& content)
	{
		this->mBody = content;
		this->AddHead("content-length", (int)content.size());
	}

	void HttpPostRequest::WriteBody(std::ostream& os)
	{
		os.write(this->mBody.c_str(), this->mBody.size());
	}

	std::shared_ptr<HttpPostRequest> HttpPostRequest::Create(const std::string& url)
	{
		std::shared_ptr<HttpPostRequest> request(new HttpPostRequest());
		return request->ParseUrl(url) ? request : nullptr;
	}

	void HttpPostRequest::AddBody(const char* data, size_t size)
	{
		this->mBody.append(data, size);
		this->AddHead("content-length", size);
	}
}

namespace Sentry
{
    HttpAsyncResponse::HttpAsyncResponse()
    {
        this->mHttpCode = 0;
        this->mContentLength = 0;
        this->mState = HttpDecodeState::FirstLine;
    }
    HttpStatus HttpAsyncResponse::OnReceiveData(asio::streambuf &streamBuffer)
    {
        std::iostream io(&streamBuffer);
        if(this->mState == HttpDecodeState::FirstLine)
        {
            this->mState = HttpDecodeState::HeadLine;
            io >> this->mVersion >> this->mHttpCode >> this->mHttpError;
            io.ignore(2); //放弃\r\n
        }
        if(this->mState == HttpDecodeState::HeadLine)
        {
            std::string lineData;
            while(std::getline(io, lineData))
            {
                if(lineData == "\r")
                {
                    this->mState = HttpDecodeState::Content;
                    break;
                }
                size_t pos = lineData.find(':');
                if (pos != std::string::npos)
                {
                    size_t length = lineData.size() - pos - 2;
                    std::string key = lineData.substr(0, pos);
					Helper::String::Tolower(key);
                    std::string val = lineData.substr(pos + 1, length);
                    this->mHeadMap.insert(std::make_pair(key, val));
                }
            }
        }
        if(this->mState == HttpDecodeState::Content)
        {
            char buffer[256] = { 0 };
            size_t size = io.readsome(buffer, 256);
            while(size > 0)
            {
                this->mContent.append(buffer, size);
                size = io.readsome(buffer, 256);
            }
        }
        return HttpStatus::CONTINUE;
    }

	bool HttpAsyncResponse::GetHead(const std::string& key, std::string& value)
	{
		auto iter = this->mHeadMap.find(key);
		if(iter != this->mHeadMap.end())
		{
			value = iter->second;
			return true;
		}
		return false;
	}
	bool HttpAsyncResponse::ToJson(std::string & json)
	{
		Json::Writer jsonWriter;
		jsonWriter.StartObject("head");
		auto iter = this->mHeadMap.begin();
		for(; iter!= this->mHeadMap.end(); iter++)
		{
			const std::string & key = iter->first;
			const std::string & value = iter->second;
			jsonWriter.AddMember(key.c_str(), value);
		}
		jsonWriter.EndObject();
		jsonWriter.AddMember("data", this->mContent);
		return jsonWriter.WriterStream(json);
	}
}

namespace Sentry
{
    HttpHandlerRequest::HttpHandlerRequest(const std::string & address)
    {
        this->mContentLength = 0;
		this->mAddress = address;
        this->mState = HttpDecodeState::FirstLine;
    }

    bool HttpHandlerRequest::GetHeadContent(const std::string &key, std::string &value)
    {
        auto iter = this->mHeadMap.find(key);
        if(iter != this->mHeadMap.end())
        {
            value = iter->second;
            return true;
        }
        return false;
    }

    HttpStatus HttpHandlerRequest::OnReceiveData(asio::streambuf &streamBuffer)
    {
        std::iostream io(&streamBuffer);
        if(this->mState == HttpDecodeState::FirstLine)
        {
            this->mState = HttpDecodeState::HeadLine;
            io >> this->mMethod >> this->mPath >> this->mVersion;
            io.ignore(2); //去掉\r\n
        }
        if(this->mState == HttpDecodeState::HeadLine)
        {
            std::string lineData;
            while(std::getline(io, lineData))
            {
                if(lineData == "\r")
                {
                    this->mState = HttpDecodeState::Content;
                    break;
                }
                size_t pos = lineData.find(":");
                if (pos != std::string::npos)
                {
                    size_t length = lineData.size() - pos - 2;
                    std::string key = lineData.substr(0, pos);

                    Helper::String::Tolower(key);
                    std::string val = lineData.substr(pos + 1, length);
                    this->mHeadMap.insert(std::make_pair(key, val));
                }
            }
            if(this->mState == HttpDecodeState::Content)
            {
                if(this->mMethod == "GET")
                {
                    size_t pos = this->mPath.find("?");
                    if(pos != std::string::npos)
                    {
                        this->mContent = mPath.substr(pos + 1);
                        this->mPath = this->mPath.substr(0, pos);
                        return HttpStatus::OK;
                    }
                }
                else if(this->mMethod == "POST")
                {
					std::string len;
					if(!this->GetHeadContent("content-length", len))
					{
						return HttpStatus::LENGTH_REQUIRED;
					}
                    this->mContentLength = std::stol(len);
                    if(this->mContentLength <= 0)
                    {
                        return HttpStatus::LENGTH_REQUIRED;
                    }
                }
                else
                {
                    return HttpStatus::METHOD_NOT_ALLOWED;
                }
            }
        }
        if(this->mState == HttpDecodeState::Content)
        {
            if(this->mContentLength == 0)
            {
                return HttpStatus::OK;
            }
            char buffer[256] = { 0 };
            size_t size = io.readsome(buffer, 256);
            while(size > 0)
            {
                this->mContent.append(buffer, size);
                if(this->mContent.size() == this->mContentLength)
                {
                    this->mState = HttpDecodeState::Finish;
                    return HttpStatus::OK;
                }
                size = io.readsome(buffer, 256);
            }
        }
        return HttpStatus::CONTINUE;
    }
}

namespace Sentry
{
	bool HttpHandlerResponse::Serailize(std::ostream& os)
	{
		os << HttpVersion << ' ' << (int)this->mCode << ' ' << HttpStatusToString(this->mCode) << "\r\n";
		for(auto iter = this->mHeadMap.begin(); iter != this->mHeadMap.end(); iter++)
		{
			const std::string & key = iter->first;
			const std::string & value = iter->second;
			os << key << ":" << value << "\r\n";
		}
		os << "\r\n";
		os.write(this->mContent.c_str(), this->mContent.size());
		return true;
	}

	bool HttpHandlerResponse::AddHead(const std::string& key, const std::string& value)
	{
		auto iter = this->mHeadMap.find(key);
		if(iter != this->mHeadMap.end())
		{
			return false;
		}
		this->mHeadMap.emplace(key, value);
		return true;
	}

	void HttpHandlerResponse::Write(HttpStatus code, const std::string& content)
	{
		this->mCode = code;
		this->mContent = content;
		this->AddHead("Content-Type", "text/plain; charset=utf-8");
		this->AddHead("Content-Length", std::to_string(content.size()));
	}

	void HttpHandlerResponse::Write(HttpStatus code, Json::Writer& jsonWriter)
	{
		this->mCode = code;
		jsonWriter.WriterStream(this->mContent);
		this->AddHead("Content-Type", "application/json; charset=utf-8");
		this->AddHead("Content-Length", std::to_string(this->mContent.size()));
	}
}