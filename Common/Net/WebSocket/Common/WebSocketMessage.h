//
// Created by 64658 on 2025/1/2.
//

#ifndef APP_WEBSOCKETMESSAGE_H
#define APP_WEBSOCKETMESSAGE_H
#include"Proto/Message/IProto.h"
#include "WebSocket/Common/WebSocket.h"
namespace ws
{
	struct Header
	{
		bool fin;
		bool mask;
		unsigned char rsv;
		unsigned int length;
		unsigned char opcode;
	};

	class Message final : public tcp::IProto
	{
	public:
		Message();
	public:
		void Clear() final;
		int OnSendMessage(std::ostream &os) final;
		int OnRecvMessage(std::istream &os, size_t size) final;
	public:
		const Header & GetHeader() const { return this->mHeader; }
		const std::string & GetMessageBody() const { return this->mMessage; }
		void SetBody(unsigned char opcode, const std::string & message, bool mask = true);
	private:
		size_t mOffset;
		Header mHeader;
		char mMaskingKey[4];
		std::string mMessage;
		char mStatus;
	};
}


#endif //APP_WEBSOCKETMESSAGE_H
