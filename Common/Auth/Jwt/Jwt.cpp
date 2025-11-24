
#include "Jwt.h"
#include "Proto/Bson/base64.h"
#ifdef __ENABLE_OPEN_SSL__
#include <openssl/evp.h>
#include <openssl/hmac.h>
namespace jwt
{
	std::string hmac_sha256(const std::string& key, const std::string& data)
	{
		unsigned int result_len;
		unsigned char result[EVP_MAX_MD_SIZE] = { 0 };
		HMAC(EVP_sha256(), key.c_str(), static_cast<int>(key.length()),
				reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), result, &result_len);
		return std::string(reinterpret_cast<char*>(result), result_len);
	}
}

#else
#include "Util/Crypt/sha1.h"
#endif

std::string jwt::secret;

std::string jwt::Encode(const std::string& payload)
{
	return jwt::Encode(payload, jwt::secret);
}

std::string jwt::Encode(const std::string& payload, const std::string& secret)
{
	std::string encoded_payload = _bson::base64::encode(payload);
#ifdef __ENABLE_OPEN_SSL__
	std::string signature = hmac_sha256(secret, encoded_payload);
#else
	std::string signature = help::Sha1::GetHMacHash(secret, encoded_payload);
#endif
	return encoded_payload + "." + _bson::base64::encode(signature);
}

std::string jwt::Encode(const json::w::Document& document, const std::string& secret)
{
	std::string payload;
	document.Serialize(&payload);
	return jwt::Encode(payload, secret);
}

bool jwt::Decode(const std::string& sign, const std::string& secret, std::string& payload)
{
	size_t pos = sign.find('.');
	if(pos == std::string::npos)
	{
		return false;
	}
	std::string first = sign.substr(0, pos);
	std::string second = sign.substr(pos + 1);
#ifdef __ENABLE_OPEN_SSL__
	std::string signature = _bson::base64::encode(hmac_sha256(secret, first));
#else
	std::string signature = _bson::base64::encode(help::Sha1::GetHMacHash(secret, first));
#endif
	if(signature != second)
	{
		return false;
	}
	payload = _bson::base64::decode(first);
	return true;
}

bool jwt::Decode(const std::string& sign, const std::string& secret, json::r::Document& document)
{
	std::string payload;
	if(!jwt::Decode(sign, secret, payload))
	{
		return false;
	}
	return document.Decode(payload);
}

bool jwt::Decode(const std::string& sign, std::string& payload)
{
	return jwt::Decode(sign, jwt::secret, payload);
}

bool jwt::Decode(const std::string& sign, json::r::Document& document)
{
	return jwt::Decode(sign, jwt::secret, document);
}