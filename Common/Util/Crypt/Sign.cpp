//
// Created by yy on 2025/11/8.
//

#include "Sign.h"

#ifdef __ENABLE_OPEN_SSL__
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include "Proto/Bson/base64.h"

namespace help
{

	inline std::string base64Encode(const std::vector<unsigned char>& data)
	{
		BIO* bio = BIO_new(BIO_s_mem());
		BIO* b64 = BIO_new(BIO_f_base64());
		bio = BIO_push(b64, bio);

		// 禁用换行符（避免 Base64 字符串含换行导致验签失败）
		BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
		BIO_write(bio, data.data(), data.size());
		BIO_flush(bio);

		char* buf = nullptr;
		long len = BIO_get_mem_data(bio, &buf);
		std::string result(buf, len);

		BIO_free_all(bio);
		return result;
	}


	inline std::string BinaryToHex(const std::string& binary)
	{
		static const char hex_chars[] = "0123456789abcdef";
		std::string hex;
		hex.reserve(binary.size() * 2);
		for (unsigned char c : binary)
		{
			hex.push_back(hex_chars[(c >> 4) & 0x0F]);
			hex.push_back(hex_chars[c & 0x0F]);
		}
		return hex;
	}

	bool sign::WithRSA(const std::string& data, const std::string& key, std::string& result)
	{
		BIO* bio = BIO_new_mem_buf(key.c_str(), (int)key.size());
		if (bio == nullptr)
		{
			return false;
		}
		EVP_PKEY* privateKey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
		BIO_free(bio);
		if (privateKey == nullptr)
		{
			return false;
		}
		EVP_MD_CTX* ctx = EVP_MD_CTX_new();
		EVP_SignInit(ctx, EVP_sha256());
		EVP_SignUpdate(ctx, data.c_str(), data.size());

		unsigned int signatureLen = 0;
		size_t size = EVP_PKEY_size(privateKey);
		std::unique_ptr<unsigned char[]> signature = std::make_unique<unsigned char[]>(size);
		{
			EVP_SignFinal(ctx, signature.get(), &signatureLen, privateKey);
			EVP_MD_CTX_free(ctx);
			result.assign((char*)signature.get(), signatureLen);
		}
		EVP_PKEY_free(privateKey);
		return true;
	}

	bool sign::WithMD5(const std::string& data, const std::string& key, std::string& result)
	{
		BIO* bio = BIO_new_mem_buf(key.c_str(), (int)key.size());
		if (bio == nullptr)
		{
			return false;
		}
		EVP_PKEY* privateKey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
		BIO_free(bio);
		if (privateKey == nullptr)
		{
			return false;
		}
		EVP_MD_CTX* ctx = EVP_MD_CTX_new();
		EVP_SignInit(ctx, EVP_md5());
		EVP_SignUpdate(ctx, data.c_str(), data.size());

		unsigned int signatureLen = 0;
		size_t size = EVP_PKEY_size(privateKey);
		std::unique_ptr<unsigned char[]> signature = std::make_unique<unsigned char[]>(size);
		{
			EVP_SignFinal(ctx, signature.get(), &signatureLen, privateKey);
			EVP_MD_CTX_free(ctx);
			result.assign((char*)signature.get(), signatureLen);
		}
		EVP_PKEY_free(privateKey);
		return true;
	}

	bool sign::Sha256WithRSA(const std::string& data, const std::string& key, std::string& result)
	{
		result.clear();
		ERR_clear_error();

		// 1. 加载私钥
		BIO* bio = BIO_new_mem_buf(key.c_str(), static_cast<int>(key.size()));
		if (bio == nullptr)
		{
			return false;
		}

		RSA* pkey = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
		BIO_free(bio);
		if (pkey == nullptr)
		{
			return false;
		}
		std::vector<unsigned char> sha256_hash(SHA256_DIGEST_LENGTH);
		SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), sha256_hash.data());

		int rsa_key_len = RSA_size(pkey);
		std::vector<unsigned char> signature(rsa_key_len);

		int sign_len = RSA_sign(
			NID_sha256,
			sha256_hash.data(),
			sha256_hash.size(),
			signature.data(),
			reinterpret_cast<unsigned int*>(&rsa_key_len),
			pkey // 私钥
		);

		if (sign_len != 1)
		{
			RSA_free(pkey);
			return false;
		}
		RSA_free(pkey);
		result = base64Encode(std::vector<unsigned char>(signature.data(), signature.data() + rsa_key_len));
		return true;
	}

	bool sign::WithSHA256(const std::string& data, const std::string& key, std::string& result)
	{
		BIO* bio = BIO_new_mem_buf(key.c_str(), (int)key.size());
		if (bio == nullptr)
		{
			return false;
		}
		EVP_PKEY* privateKey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
		BIO_free(bio);
		if (privateKey == nullptr)
		{
			return false;
		}
		EVP_MD_CTX* ctx = EVP_MD_CTX_new();
		EVP_SignInit(ctx, EVP_sha256());
		EVP_SignUpdate(ctx, data.c_str(), data.size());

		unsigned int signatureLen = 0;
		size_t size = EVP_PKEY_size(privateKey);
		std::unique_ptr<unsigned char[]> signature = std::make_unique<unsigned char[]>(size);
		{
			EVP_SignFinal(ctx, signature.get(), &signatureLen, privateKey);
			EVP_MD_CTX_free(ctx);
			result = _bson::base64::encode((char*)signature.get(), signatureLen);
		}
		EVP_PKEY_free(privateKey);
		return true;
	}
}

#endif