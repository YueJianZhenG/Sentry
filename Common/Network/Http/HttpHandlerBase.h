#pragma once
#include <XCode/XCode.h>
#include <Define/CommonTypeDef.h>
#include <Network/Http/Http.h>
namespace GameKeeper
{
	class HttpHandlerBase
	{
	public:
		HttpHandlerBase() = default;
		virtual ~HttpHandlerBase() = default;
	public:
		virtual bool WriterToBuffer(std::ostream & os) { return true; }
        virtual bool OnReceiveBody(asio::streambuf & buf) = 0;
		virtual bool OnSessionError(const asio::error_code & code) = 0;
		virtual bool OnReceiveHeard(asio::streambuf & buf, size_t size) = 0;
	protected:
		void ParseHeard(asio::streambuf & buf, size_t size);
		bool GetHeardData(const std::string & key, std::string & value);
	private:
		std::unordered_map<std::string, std::string> mHeardMap;
    protected:
        char mHandlerBuffer[1024];
	};
}