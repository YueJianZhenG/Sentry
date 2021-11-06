#pragma once
#include "HttpRequest.h"
namespace GameKeeper
{
	class HttpReadContent;
	class HttpClientComponent;
	class HttpGetRequest : public HttpRequest
	{
	public:
		explicit HttpGetRequest(HttpClientComponent * component);
		~HttpGetRequest() override = default;
	public:
        HttpMethodType GetType() final { return HttpMethodType::GET; }
        XCode Get(const std::string & url, HttpReadContent & response);
	protected:
		bool WriterToBuffer(std::ostream & os) override;
        void OnReceiveBody(asio::streambuf & buf) override;
	private:
        HttpReadContent * mResponse;
#ifdef __DEBUG__
		size_t mCurrentLength;
#endif // __DEBUG__

	};
}