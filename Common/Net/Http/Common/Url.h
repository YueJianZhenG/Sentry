//
// Created by yy on 2023/12/10.
//

#ifndef APP_URL_H
#define APP_URL_H
#include"Http/Common/Content.h"
namespace http
{
	namespace url
	{
		extern std::string encode(const std::string & url);
		extern std::string decode(const std::string & url);
	}

	class Url : public tcp::IProto
	{
	public:
		Url();
		explicit Url(const char * method);
		int OnSendMessage(std::ostream &os) final;
		int OnRecvMessage(std::istream &os, size_t size) final;
	public:
		void Clear() final;
		bool Decode(const std::string & url);
	public:
		std::string ToString() final { return this->mUrl; }
		inline http::FromContent * Query() { return &this->mQuery;}
		inline const std::string & ToStr() const { return this->mUrl; }
		inline bool IsHttps() const { return this->mProto == "https"; }
		inline const std::string & Path() const { return this->mPath; }
		inline const std::string & Host() const { return this->mHost; }
		inline const std::string & Port() const { return this->mPort; }
		inline const std::string & Method() const { return this->mMethod; }
		inline const std::string & Protocol() const { return this->mProto; }
		inline const std::string & Version() const { return this->mVersion; }
		inline const http::FromContent & GetQuery() const { return this->mQuery; }
	private:
		int mReadCount;
		std::string mUrl;
		std::string mPath;
		std::string mHost;
		std::string mPort;
		std::string mProto;
		std::string mMethod;
		std::string mVersion;
		http::FromContent mQuery;
	};
}


#endif //APP_URL_H
