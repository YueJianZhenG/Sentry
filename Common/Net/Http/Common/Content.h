//
// Created by yy on 2023/11/19.
//

#ifndef APP_CONTENT_H
#define APP_CONTENT_H
#include<fstream>
#include<map>
#include <utility>
#include "IContent.h"
#include"Proto/Message/IProto.h"
#include"XML/Document/XDocument.h"
#include<Yyjson/Document/Document.h>

namespace http
{
    class FromContent final : public Content
    {
    public:
        inline void Clear() { this->mParameters.clear(); }
        auto End() const { return this->mParameters.end(); }
        auto Begin() const { return this->mParameters.begin(); }
        size_t Size() const { return this->mParameters.size(); }
    public:
		std::string ToStr() const final;
		void WriteToLua(lua_State *l) final;
		bool Add(const std::string & k, int v);
        bool Add(const std::string & k, const std::string & v);
	public:
		void Set(const std::string & k, int v);
		void Set(const std::string & k, const std::string & v);
    public:
		std::string Serialize() const;
		bool Decode(const std::string & content);
        bool Get(std::vector<std::string> & keys) const;
        bool Get(const std::string & k, int & value) const;
		bool Get(const std::string & k, long long & value) const;
		bool Get(const std::string & key, std::string & value) const;
        http::ContentType GetContentType() const final { return http::ContentType::FROM; }
    private:
        bool OnDecode() final;
        void OnWriteHead(std::ostream &os) final;
        int OnWriteBody(std::ostream &os) final;
        int OnRecvMessage(std::istream & ss, size_t size) final;
		size_t ContentLength() final { return this->mContent.size();}
		inline void SetContentLength(long long len) final { this->mContent.reserve(len); }
    private:
        std::string mContent;
        std::map<std::string, std::string> mParameters;
    };
}

namespace http
{
    class JsonContent final : public Content
    {
	public:
		explicit JsonContent() = default;
		explicit JsonContent(std::string  json) : mJson(std::move(json)) { }
		explicit JsonContent(const char * json, size_t size) : mJson(json, size) { }
		~JsonContent() override = default;
    public:
		void Write(const json::w::Document & document);
		inline const std::string & JsonStr() const { return this->mJson;}
		inline const json::r::Document & JsonObject() const { return this->mDocument; }
		inline http::ContentType GetContentType() const final { return http::ContentType::JSON; }

		template<typename T>
		inline bool Get(const char * key, T & value) const
		{
			return this->mDocument.Get(key, value);
		}
    private:
		void WriteToLua(lua_State *l) final;
        void OnWriteHead(std::ostream &os) final;
        int OnWriteBody(std::ostream &os) final;
        int OnRecvMessage(std::istream & is, size_t size) final;
		inline size_t ContentLength() final { return this->mJson.size(); }
    	inline std::string ToStr() const final { return this->mDocument.ToString(); }
		inline void SetContentLength(long long len) final { this->mJson.reserve(len); }
		inline bool OnDecode() final { return this->mDocument.Decode(this->mJson, YYJSON_READ_INSITU); }
	private:
        std::string mJson;
		json::r::Document mDocument;
	};
}

namespace http
{
	class XMLContent final : public Content, public xml::XDocument
	{
	public:
		explicit XMLContent() = default;
		explicit XMLContent(std::string  xml) : mXml(std::move(xml)) { }
		explicit XMLContent(const char* xml, size_t size) : mXml(xml, size) { }
		~XMLContent() override = default;
	public:
		http::ContentType GetContentType() const final { return http::ContentType::XML; }
	private:
		void WriteToLua(lua_State* l) final;
		void OnWriteHead(std::ostream& os) final;
		int OnWriteBody(std::ostream& os) final;
		int OnRecvMessage(std::istream& is, size_t size) final;
		bool OnDecode() final { return this->Decode(this->mXml); }
		std::string ToStr() const final { return this->mXml; }
		inline size_t ContentLength() final { return this->mXml.size(); }
		inline void SetContentLength(long long len) final { this->mXml.reserve(len); }
	private:
		std::string mXml;
	};
}

namespace http
{
	class TextContent final : public Content
	{
	public:
		explicit TextContent() : mMaxSize(0) { }
		explicit TextContent(std::string t) : mMaxSize(0), mConType(std::move(t)) {}
	private:
		void WriteToLua(lua_State *l) override;
		bool OnDecode() final { return true; }
		int OnWriteBody(std::ostream &os) final;
		void OnWriteHead(std::ostream &os) final;
		int OnRecvMessage(std::istream & is, size_t size) final;
		std::string ToStr() const final { return this->mContent; }
		inline size_t ContentLength() final { return this->mContent.size();}
	public:
		void Append(const std::string & content);
		std::string * Data() { return &this->mContent;}
		const std::string &Content() const { return this->mContent; }
		inline void SetMaxSize(size_t size) { this->mMaxSize = size; }
		void SetContent(const std::string & type, const std::string & content);
		inline void SetContentType(const std::string & t) { this->mConType = t; }
		void SetContent(const std::string & type, const char * content, size_t size);
		http::ContentType GetContentType() const final { return http::ContentType::TEXT; }
		inline void SetContentLength(long long len) final { this->mContent.reserve(len); }
	private:
		size_t mMaxSize;
		std::string mUrl;
		std::string mConType;
		std::string mContent;
	};
}

namespace http
{
	class FileContent final : public Content
	{
	public:
		FileContent();
		explicit FileContent(std::string t);
		~FileContent() override;
	public:
		bool OpenFile(const std::string & path);
		bool MakeFile(const std::string & path);
		inline void SetMaxSize(size_t size) { this->mMaxSize = size; }
		bool OpenFile(const std::string & path, const std::string & type);
		inline http::ContentType GetContentType() const final { return http::ContentType::FILE; }
	private:
		bool OnDecode() final;
		void WriteToLua(lua_State *l) final;
		int OnWriteBody(std::ostream &os) final;
		void OnWriteHead(std::ostream &os) final;
		int OnRecvMessage(std::istream & is, size_t) final;
		inline std::string ToStr() const final { return this->mPath; }
		inline size_t ContentLength() final { return this->mFileSize;}
	public:
		inline const std::string & Path() const { return this->mPath; }
	private:
		size_t mMaxSize;
		size_t mFileSize;
		size_t mSendSize;
		std::string mPath;
		std::string mType;
		std::fstream mFile;
	};
}

namespace http
{
	class ChunkedContent final : public Content
	{
	public:
		ChunkedContent() : mIndex(0), mCount(0) { }
		~ChunkedContent() override { this->mContent.clear(); }
	private:
		bool OnDecode() final;
		void WriteToLua(lua_State *l) final;
		int OnWriteBody(std::ostream &os) final;
		void OnWriteHead(std::ostream &os) final;
		int OnRecvMessage(std::istream & is, size_t size) final;
		std::string ToStr() const final { return this->mContent; }
		inline size_t ContentLength() final { return 0; }
		http::ContentType GetContentType() const final { return http::ContentType::CHUNKED; }
	public:
		void SetContent(const std::string & content);
		void SetContent(const std::string & type, const std::string & content);
		const std::string & GetContent() const { return this->mContent; }
	private:
		size_t mIndex;
		size_t mCount;
		std::string mContent;
		std::string mContType;
	};
}

namespace http
{
	class MultipartFromContent final : public Content
	{
	public:
		MultipartFromContent();
		~MultipartFromContent() final { this->mFile.close(); }
	public:
		bool OnDecode() final;
		void WriteToLua(lua_State *l) final;
		int OnRecvMessage(std::istream & buffer, size_t size) final;
		void Init(const std::string & dir, size_t limit = 1024 * 1024 * 2); //最大2M
	public:
		inline bool IsDone() const { return this->mDone; }
		inline const std::string & Path() const { return this->mPath; }
		inline const std::string & FileName() const { return this->mFileName; }
		inline const std::string & ContentType() const { return this->mContType; }
	public:
		bool Add(const std::string& k, const std::string& v);
	private:
		size_t GetContentLength() const;
		std::string ToStr() const final;
		int OnWriteBody(std::ostream& os) final;
		void OnWriteHead(std::ostream& os) final;
		inline size_t ContentLength() final { return this->mReadCount;}
		inline http::ContentType GetContentType() const final { return http::ContentType::MULTIPAR; }
	private:
		bool mDone;
		size_t mLength;
		size_t mMaxCount;
		std::string mDir;
		std::string mPath;
		size_t mReadCount;
		std::fstream mFile;
		std::string mFileName;
		std::string mFieldName;
		std::string mContType;
		std::string mBoundary;
		std::string mContent;
		std::vector<std::string> mHeader;
		std::map<std::string, std::string> mFromData;
	};
}

namespace http
{
	class BinContent final : public Content
	{
	public:
		BinContent();
	private:
		void WriteToLua(lua_State *l) final;
		bool OnDecode() final { return true; }
		int OnWriteBody(std::ostream &os) final;
		void OnWriteHead(std::ostream &os) final;
		int OnRecvMessage(std::istream &is, size_t size) final;
		inline size_t ContentLength() final { return this->mBody.size();}
		inline void SetContentLength(long long len) final { this->mBody.reserve(len); }
		http::ContentType GetContentType() const final { return http::ContentType::PB; }
	public:
		const std::string & GetContent() const { return this->mBody; }
	private:
		std::string mBody;
	};
}

namespace http
{
	class StringContent final : public Content
	{
	public:
		StringContent(const std::string & str) : mBody(str) { };
	private:
		void WriteToLua(lua_State* l) final;
		bool OnDecode() final { return true; }
		void OnWriteHead(std::ostream& os) final { }
		int OnWriteBody(std::ostream& os) final;
		int OnRecvMessage(std::istream& is, size_t size) final;
		size_t ContentLength() final { return this->mBody.size(); }
	public:
		const std::string & GetContent() const { return this->mBody; }
		http::ContentType GetContentType() const final { return http::ContentType::PB; }
	private:
		std::string mBody;
	};
}

#endif //APP_CONTENT_H
