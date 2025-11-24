
#pragma once

#ifdef __ENABLE_OPEN_SSL__
#include <string>
namespace help
{
	namespace sign
	{
		extern bool WithRSA(const std::string& data, const std::string& key, std::string& result);
		extern bool WithMD5(const std::string& data, const std::string& key, std::string& result);
		extern bool WithSHA256(const std::string& data, const std::string& key, std::string& result);
		extern bool Sha256WithRSA(const std::string& data, const std::string& key, std::string& result);
	}
}

#endif