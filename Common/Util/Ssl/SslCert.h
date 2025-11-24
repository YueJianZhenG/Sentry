//
// Created by 64658 on 2025/10/15.
//


#ifndef APP_SSLCERT_H
#define APP_SSLCERT_H
#include <string>
namespace help
{
#ifdef __ENABLE_OPEN_SSL__
	namespace ssl
	{
		class CertInfo
		{
		public:
			std::string Subject; //主题
			std::string Issuer; //颁发者
			std::string CommonName; //域名
			std::string NotBefore; //生效时间
			std::string NotAfter; //过期时间
			long long ExpTime = 0; //过期时间戳
			long long BeginTime = 0; //开始时间
		};
		extern bool IsPublicKey(const std::string & key);
		extern bool IsPrivateKey(const std::string & key);
		extern bool Decode(const std::string & cert, ssl::CertInfo & certInfo);
	}
#endif
}

#endif //APP_SSLCERT_H
