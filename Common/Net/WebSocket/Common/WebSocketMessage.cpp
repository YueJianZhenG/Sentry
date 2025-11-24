//
// Created by 64658 on 2025/1/2.
//

#include "WebSocketMessage.h"
#include "Util/Tools/Random.h"

namespace ws
{

	constexpr int CHAR_COUNT = std::numeric_limits<char>::max();
	constexpr int USHORT_COUNT = std::numeric_limits<unsigned short>::max();

	Message::Message()
	{
		this->Clear();
	}

	void Message::Clear()
	{
		this->mOffset = 0;
		this->mMessage.clear();
		this->mStatus = tcp::Decode::None;
		memset(&this->mHeader, 0, sizeof(Header));
		memset(this->mMaskingKey, 0, sizeof(this->mMaskingKey));
	}

	int Message::OnSendMessage(std::ostream& os)
	{
		this->mHeader.fin = true; // 完整帧
		this->mHeader.rsv = 0;    // RSV 保持为 0
		//this->mHeader.mMask = false;  // 客户端消息 Mask 位应为 1，服务器发送时可为 0

		std::unique_ptr<char[]> buffer;

		if (this->mMessage.size() <= CHAR_COUNT - 2)
		{
			this->mHeader.length = this->mMessage.size(); // 直接使用消息大小
		}
		else if (this->mMessage.size() <= USHORT_COUNT)
		{
			this->mHeader.length = 126;
			buffer = std::make_unique<char[]>(2);  // 使用 2 个字节表示长度
			unsigned short size = this->mMessage.size();
			tcp::Data::Write<unsigned short>(buffer.get(), size);
		}
		else
		{
			this->mHeader.length = 127;
			buffer = std::make_unique<char[]>(8);  // 使用 8 个字节表示长度
			unsigned long long size = this->mMessage.size();
			tcp::Data::Write<unsigned long long>(buffer.get(), size);
		}

		// 构建第一个字节 (FIN + RSV + Opcode)
		unsigned char firstByte = 0x00;
		firstByte |= 0x80; // 设置 FIN 位
		firstByte |= (this->mHeader.opcode & 0x0F); // 设置操作码（例如 0x1 表示文本）

		os << firstByte;

		// 写入 Mask 位和消息长度
		unsigned char maskAndLengthByte = 0;
		if (this->mHeader.mask)
		{
			maskAndLengthByte |= 0x80;
		}
		maskAndLengthByte |= this->mHeader.length;
		os << maskAndLengthByte;

		if (this->mHeader.length == 126)
		{
			os.write(buffer.get(), 2);
		}
		else if (this->mHeader.length == 127)
		{
			os.write(buffer.get(), 8);
		}
		if (this->mHeader.mask)
		{
			os.write(this->mMaskingKey, 4);
		}
		os.write(this->mMessage.c_str(), this->mMessage.size());
		return 0;
	}

	void Message::SetBody(unsigned char opcode, const std::string& message, bool mask)
	{
		this->mHeader.mask = mask;
		this->mHeader.opcode = opcode;
		this->mMessage.assign(message);
		if (this->mHeader.mask)
		{
			for (size_t index = 0; index < sizeof(this->mMaskingKey); index++)
			{
				int num = help::Rand::Random<int>(0, sizeof(unsigned char));
				this->mMaskingKey[index] = static_cast<char>(num);
			}
			for (size_t index = 0; index < this->mMessage.size(); index++)
			{
				this->mMessage[index] = this->mMessage[index] ^ this->mMaskingKey[index % 4];
			}
		}
	}

	int Message::OnRecvMessage(std::istream& os, size_t size)
	{
		if (this->mStatus == tcp::Decode::None)
		{
			this->mOffset = 2;
			int value = os.get();
			int value2 = os.get();
			this->mHeader.fin = (value & 0x80) != 0;
			this->mHeader.opcode = value & 0x0F;
			this->mHeader.mask = (value2 & 0x80) != 0;
			this->mHeader.length = value2 & 0x7F;
			if (!this->mHeader.fin) //只支持完整包
			{
				return tcp::read::decode_error;
			}
			size_t readCount = 0;
			switch (this->mHeader.length)
			{
				case 126:
					readCount += 2;
					break;
				case 127:
					readCount += 8;
					break;
			}
			if (this->mHeader.mask)
			{
				readCount += 4;
			}
			this->mStatus = tcp::Decode::MessageHead;
			if (size - this->mOffset < readCount)
			{
				this->mOffset = 0;
				return (int)readCount;
			}
		}
		if (this->mStatus == tcp::Decode::MessageHead)
		{
			switch (this->mHeader.length)
			{
				case 126:
				{
					this->mOffset += 2;
					if (size < this->mOffset)
					{
						return tcp::read::decode_error;
					}
					char buffer[2] = { 0 };
					if (os.readsome(buffer, sizeof(buffer)) != 2)
					{
						return tcp::read::decode_error;
					}
					unsigned short len = 0;
					tcp::Data::Read(buffer, len);
					this->mHeader.length = len;
					break;
				}
				case 127:
				{
					this->mOffset += 8;
					if (size < this->mOffset)
					{
						return tcp::read::decode_error;
					}
					char buffer[8] = { 0 };
					os.readsome(buffer, sizeof(buffer));
					unsigned long long len = 0;
					tcp::Data::Read(buffer, len);
					this->mHeader.length = len;
					break;
				}
			}
			if (this->mHeader.length > ws::MESSAGE_MAX_COUNT)
			{
				return tcp::read::big_long;
			}

			if (this->mHeader.mask)
			{
				this->mOffset += 4;
				if (size < this->mOffset)
				{
					return tcp::read::decode_error;
				}
				memset(this->mMaskingKey, 0, 4);
				os.readsome(this->mMaskingKey, 4);
			}
			this->mOffset = 0;
			this->mStatus = tcp::Decode::MessageBody;
			this->mMessage.resize(this->mHeader.length);
		}
		int maxReadCount = this->mHeader.length - this->mOffset;
		char* buffer = const_cast<char*>(this->mMessage.c_str());
		size_t count = os.readsome(buffer + this->mOffset, maxReadCount);
		while (count > 0)
		{
			this->mOffset += count;
			maxReadCount = this->mHeader.length - this->mOffset;
			count = os.readsome(buffer + this->mOffset, maxReadCount);
		}
		if (this->mHeader.length - this->mOffset == 0)
		{
			if (this->mHeader.mask)
			{
				for (size_t i = 0; i < this->mMessage.size(); ++i)
				{
					this->mMessage[i] ^= this->mMaskingKey[i % 4];
				}
			}
			return tcp::read::done;
		}
		return tcp::read::some;
	}
}