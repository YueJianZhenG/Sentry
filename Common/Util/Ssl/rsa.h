
#pragma once
#ifdef __ENABLE_OPEN_SSL__

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <string>
#include <stdexcept>
#include <fstream>
#include <memory>

namespace ssl
{
	class RSAEncryptor
	{
	public:
		explicit RSAEncryptor() = default;
		~RSAEncryptor() { printf("&&&&&&&&&&&&&&&&&");}
		bool Encode(const std::string& plainText, std::string& encryptedText)
		{
			if (!publicKey)
			{
				return false;
			}

			encryptedText.assign(RSA_size(publicKey.get()), '\0');
			int resultLen = RSA_public_encrypt(
					(int)plainText.size(),
					reinterpret_cast<const unsigned char*>(plainText.c_str()),
					reinterpret_cast<unsigned char*>(&encryptedText[0]),
					publicKey.get(),
					RSA_PKCS1_OAEP_PADDING
			);

			if (resultLen == -1)
			{
				return false;
			}
			encryptedText.resize(resultLen); // 调整为实际加密后的大小
			return true;
		}

		bool Decode(const std::string& cipherText, std::string& decryptedText)
		{
			if (!privateKey)
			{
				return false;
			}

			decryptedText.assign(RSA_size(privateKey.get()), '\0');
			int resultLen = RSA_private_decrypt(
					(int)cipherText.size(),
					reinterpret_cast<const unsigned char*>(cipherText.c_str()),
					reinterpret_cast<unsigned char*>(&decryptedText[0]),
					privateKey.get(),
					RSA_PKCS1_OAEP_PADDING
			);

			if (resultLen == -1)
			{
				return false;
			}
			decryptedText.resize(resultLen); // 调整为实际解密后的大小
			return true;
		}

	public:
		bool Init(const std::string& publicKeyFile, const std::string& privateKeyFile, int size = 2048)
		{
			if (!loadKeysFromFile(publicKeyFile, privateKeyFile))
			{
				if (size > 0)
				{
					this->generateKeys(size);
					return this->saveKeysToFile(publicKeyFile, privateKeyFile);
				}
				return false;
			}
			return true;
		}

	private:
		std::unique_ptr<RSA, decltype(&::RSA_free)> publicKey{ nullptr, ::RSA_free };
		std::unique_ptr<RSA, decltype(&::RSA_free)> privateKey{ nullptr, ::RSA_free };


		void generateKeys(int keySize)
		{
			std::unique_ptr<BIGNUM, decltype(&::BN_free)> bn(BN_new(), ::BN_free);
			BN_set_word(bn.get(), RSA_F4);

			std::unique_ptr<RSA, decltype(&::RSA_free)> rsa(RSA_new(), ::RSA_free);
			RSA_generate_key_ex(rsa.get(), keySize, bn.get(), nullptr);

			publicKey.reset(RSAPublicKey_dup(rsa.get()));
			privateKey.reset(RSAPrivateKey_dup(rsa.get()));
		}

		bool saveKeysToFile(const std::string& publicKeyFile, const std::string& privateKeyFile) const
		{
			if (publicKey)
			{
				std::ofstream pubFile(publicKeyFile, std::ios::binary);
				if (!pubFile)
				{
					return false;
				}

				BIO* pubBio = BIO_new(BIO_s_mem());
				PEM_write_bio_RSAPublicKey(pubBio, publicKey.get());

				BUF_MEM* pubBuf;
				BIO_get_mem_ptr(pubBio, &pubBuf);
				pubFile.write(pubBuf->data, (int)pubBuf->length);

				BIO_free(pubBio);
			}

			if (privateKey)
			{
				std::ofstream privFile(privateKeyFile, std::ios::binary);
				if (!privFile)
				{
					return false;
				}

				BIO* privBio = BIO_new(BIO_s_mem());
				PEM_write_bio_RSAPrivateKey(privBio, privateKey.get(), nullptr, nullptr, 0, nullptr, nullptr);

				BUF_MEM* privBuf;
				BIO_get_mem_ptr(privBio, &privBuf);
				privFile.write(privBuf->data, (int)privBuf->length);

				BIO_free(privBio);
			}
			return true;
		}

		bool loadKeysFromFile(const std::string& publicKeyFile, const std::string& privateKeyFile)
		{
			std::ifstream pubFile(publicKeyFile, std::ios::binary);
			if (pubFile)
			{
				BIO* pubBio = BIO_new(BIO_s_mem());
				std::string pubKeyStr((std::istreambuf_iterator<char>(pubFile)), std::istreambuf_iterator<char>());
				BIO_write(pubBio, pubKeyStr.data(), (int)pubKeyStr.size());

				publicKey.reset(PEM_read_bio_RSAPublicKey(pubBio, nullptr, nullptr, nullptr));
				BIO_free(pubBio);
			}

			std::ifstream privFile(privateKeyFile, std::ios::binary);
			if (privFile)
			{
				BIO* privBio = BIO_new(BIO_s_mem());
				std::string privKeyStr((std::istreambuf_iterator<char>(privFile)), std::istreambuf_iterator<char>());
				BIO_write(privBio, privKeyStr.data(), (int)privKeyStr.size());

				privateKey.reset(PEM_read_bio_RSAPrivateKey(privBio, nullptr, nullptr, nullptr));
				BIO_free(privBio);
			}

			return publicKey && privateKey;
		}
	};
}

#endif