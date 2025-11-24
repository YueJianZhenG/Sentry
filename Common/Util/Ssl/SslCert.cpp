//
// Created by 64658 on 2025/10/15.
//

#include <ctime>
#include <iomanip>
#include "SslCert.h"

#include "Util/Tools/String.h"
#ifdef __ENABLE_OPEN_SSL__
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/asn1.h>
#include <chrono>
#include "Util/Tools/TimeHelper.h"
#include "openssl/x509v3.h"
#endif

namespace help
{
#ifdef __ENABLE_OPEN_SSL__
	namespace ssl
	{
// 将 ASN1_TIME 转为 time_t（计算天数要用）
		inline long long ASN1_TIME_to_time_t(const ASN1_TIME* time)
		{
			struct tm t = {};
			const char* str = (const char*)time->data;
			size_t len = time->length;

			// ASN1_TIME 格式通常是 YYYYMMDDHHMMSSZ
			if (time->type == V_ASN1_UTCTIME)
			{
				if (len < 13) return 0;
				if(!help::Str::Scanf(str, "%2d%2d%2d%2d%2d%2d",
						&t.tm_year, &t.tm_mon, &t.tm_mday,
						&t.tm_hour, &t.tm_min, &t.tm_sec))
				{
					return 0;
				}
				t.tm_year += (t.tm_year < 70 ? 100 : 0); // UTCTIME < 2000 的特殊处理
			}
			else if (time->type == V_ASN1_GENERALIZEDTIME)
			{
				if (len < 15) { return 0; }
				if(!help::Str::Scanf(str, "%4d%2d%2d%2d%2d%2d",
						&t.tm_year, &t.tm_mon, &t.tm_mday,
						&t.tm_hour, &t.tm_min, &t.tm_sec))
				{
					return 0;
				}
				t.tm_year -= 1900;
			}
			t.tm_mon -= 1;
			t.tm_isdst = 0;
			//return std::get_time(&t, "%Y-%m-%d %H:%M:%S");
			std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(std::mktime(&t));
			return (long long)std::chrono::system_clock::to_time_t(tp);
		}

// 获取 CN（Common Name）
		inline bool GetCNFromX509(X509* cert, std::string & name)
		{
			if(cert == nullptr)
			{
				return false;
			}
			X509_NAME* subj = X509_get_subject_name(cert);
			if(subj == nullptr)
			{
				return false;
			}
			char cn[256] = { 0 };
			int len = X509_NAME_get_text_by_NID(subj, NID_commonName, cn, sizeof(cn));
			if(len <= 0)
			{
				return false;
			}
			name.assign(cn, len);
			return true;
		}

		inline bool GetSANFromX509(X509* cert, std::string & result) {
			if(cert == nullptr)
			{
				return false;
			}
			std::string san;
			// 获取 SAN 扩展（NID_subject_alt_name 是扩展标识符）
			STACK_OF(GENERAL_NAME)* san_names = static_cast<STACK_OF(GENERAL_NAME)*>(
					X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr)
			);
			if (san_names == nullptr) {
				return false;
			}

			// 遍历 SAN 中的所有域名（只处理 DNS 类型）
			for (int i = 0; i < sk_GENERAL_NAME_num(san_names); ++i) {
				GENERAL_NAME* name = sk_GENERAL_NAME_value(san_names, i);
				if (name->type == GEN_DNS) { // 只关注 DNS 类型的 SAN
					char* dns_name = (char*)ASN1_STRING_get0_data(name->d.dNSName);
					if (dns_name) {
						if(!result.empty())
						{
							result += ';';
						}
						result.append(dns_name);
					}
				}
			}
			sk_GENERAL_NAME_free(san_names); // 释放 SAN 资源
			return true;
		}
	}

	bool ssl::Decode(const std::string& certPath, ssl::CertInfo& certInfo)
	{
		BIO * bio = BIO_new_file(certPath.c_str(), "rb");
		if(bio == nullptr)
		{
			return false;
		}
		// 从 PEM 文件读取证书（PEM_read_X509 自动处理 PEM 格式的 BEGIN/END 标记）
		X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
		{
			char subject[512] = { 0};
			X509_NAME_oneline(X509_get_subject_name(cert), subject, sizeof(subject));

			certInfo.Subject.assign(subject);
			GetCNFromX509(cert, certInfo.Issuer);
			GetSANFromX509(cert, certInfo.CommonName);
			certInfo.ExpTime = ASN1_TIME_to_time_t(X509_get_notAfter(cert));
			certInfo.BeginTime = ASN1_TIME_to_time_t(X509_get_notBefore(cert));
			certInfo.NotAfter = help::Time::GetDateString(certInfo.ExpTime);
			certInfo.NotBefore = help::Time::GetDateString(certInfo.BeginTime);
		}
		X509_free(cert);
		BIO_free_all(bio);
		return true;
	}

	extern bool ssl::IsPrivateKey(const std::string & pem_str)
	{
		BIO* bio = nullptr;
		EVP_PKEY* pkey = nullptr;
		bool result = false;
		do
		{
			bio = BIO_new_mem_buf(pem_str.c_str(), static_cast<int>(pem_str.size()));
			if (bio == nullptr)
			{
				return false;
			}
			pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
			if (pkey == nullptr)
			{
				break;
			}
			result = true;
		}
		while(false);
		BIO_free(bio);
		EVP_PKEY_free(pkey);
		return result;
	}

	bool ssl::IsPublicKey(const std::string & pem_str)
	{
		BIO* bio = nullptr;
		EVP_PKEY* pkey = nullptr;
		bool result = false;

		do
		{
			bio = BIO_new_mem_buf(pem_str.c_str(), static_cast<int>(pem_str.size()));
			if (bio == nullptr)
			{
				break; // 内存分配失败，直接返回 false
			}

			pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
			if (pkey == nullptr)
			{
				break;
			}
			result = true; // 所有检测通过，确认为有效公钥
		}
		while (false);

		if (bio != nullptr)
		{
			BIO_free_all(bio);
		}
		if (pkey != nullptr)
		{
			EVP_PKEY_free(pkey);
		}

		return result;
	}
#endif
}