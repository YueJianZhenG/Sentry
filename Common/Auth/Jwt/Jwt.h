//
// Created by yy on 2024/1/1.
//

#ifndef APP_JWT_H
#define APP_JWT_H

#include "Yyjson/Document/Document.h"
namespace jwt
{
	extern std::string secret;
	extern std::string Encode(const std::string& payload);
	extern std::string Encode(const std::string& payload, const std::string& secret);
	extern std::string Encode(const json::w::Document & document, const std::string& secret);

	extern bool Decode(const std::string& sign, std::string & payload);
	extern bool Decode(const std::string& sign, json::r::Document & document);
	extern bool Decode(const std::string& sign, const std::string& secret, std::string & payload);
	extern bool Decode(const std::string& sign, const std::string& secret, json::r::Document & document);
}

#endif //APP_JWT_H
