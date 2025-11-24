//
// Created by zmhy0073 on 2022/9/27.
//

#include"Message.h"
#include"Proto/Include/Message.h"
#include"Yyjson/Document/Document.h"
#include"Util/Tools/String.h"

namespace rpc
{
	const std::string& Head::GetStr(const std::string& key) const
	{
		static std::string empty;
		auto iter = std::find_if(this->mHeader.begin(), this->mHeader.end(),
		                         [&key](const std::pair<std::string, std::string>& item)
		                         {
			                         return item.first == key;
		                         });
		return iter != this->mHeader.end() ? iter->second : empty;
	}

	int Head::OnRecvMessage(std::istream& os, size_t size)
	{
		size_t len = 0;
		this->mHeader.clear();
		std::string line, key, value;
		while (std::getline(os, line))
		{
			len += (line.size() + 1);
			if (line.empty())
			{
				break;
			}
			if (help::Str::Split(line, ':', key, value) != 0)
			{
				return -1;
			}
			this->mHeader.emplace_back(key, value);
			line.clear();
		}
		return (int)len;
	}

	size_t Head::GetLength() const
	{
		size_t len = 0;
		auto iter = this->mHeader.begin();
		for (; iter != this->mHeader.end(); iter++)
		{
			len += (iter->first.size() + 1);
			len += (iter->second.size() + 1);
		}
		return len + 1;
	}

	int Head::OnSendMessage(std::ostream& os)
	{
		auto iter = this->mHeader.begin();
		for (; iter != this->mHeader.end(); iter++)
		{
			const std::string& key = iter->first;
			const std::string& val = iter->second;
			os.write(key.c_str(), (int)key.size()) << ':';
			os.write(val.c_str(), (int)val.size()) << '\n';
		}
		os << '\n';
		return 0;
	}

	void Message::Init(const rpc::ProtoHead& protoHead)
	{
		this->mProtoHead = protoHead;
	}

	int Message::OnRecvMessage(std::istream& os, size_t size)
	{
		switch (this->mMsg)
		{
		case rpc::msg::bin:
			{
				//tcp::Data::ReadHead(os, this->mProtoHead, true);
				int len = this->mHead.OnRecvMessage(os, size);
				if (len <= 0)
				{
					return tcp::read::error;
				}
				this->mBody.clear();
				this->mProtoHead.Len -= len;
				if (this->mProtoHead.Len == 0)
				{
					return tcp::read::done;
				}
				this->mBody.resize(this->mProtoHead.Len);
				char* buffer = const_cast<char*>(this->mBody.c_str());
				size_t count = os.readsome(buffer, this->mProtoHead.Len);
				if (count != this->mProtoHead.Len)
				{
					return tcp::read::decode_error;
				}
				return tcp::read::done;
			}
		case rpc::msg::text:
			{
				constexpr size_t protoLen = rpc::RPC_PACK_HEAD_LEN - rpc::RPC_PACKET_LEN_BYTES;

				this->mProtoHead.Len = size - protoLen;
				tcp::Data::ReadHead(os, this->mProtoHead, false);
				int len = this->mHead.OnRecvMessage(os, size);
				if (len <= 0)
				{
					return tcp::read::error;
				}
				this->mBody.clear();
				this->mProtoHead.Len -= len;
				if (this->mProtoHead.Len == 0)
				{
					return tcp::read::done;
				}
				this->mBody.resize(this->mProtoHead.Len);
				char* buffer = const_cast<char*>(this->mBody.c_str());
				size_t count = os.readsome(buffer, this->mProtoHead.Len);
				if (count != this->mProtoHead.Len)
				{
					return tcp::read::decode_error;
				}
				return tcp::read::done;
			}
		case rpc::msg::json:
			{
				this->mBody.resize(size);
				char* buffer = (char*)this->mBody.data();
				if (os.readsome(buffer, (int)size) != size)
				{
					return tcp::read::error;
				}
				json::r::Document document;
				if (!document.Decode(buffer, size))
				{
					return tcp::read::decode_error;
				}
				json::r::Value jsonValue;
				document.Get("type", this->mProtoHead.type);
				document.Get("proto", this->mProtoHead.porto);
				document.Get("rpc_id", this->mProtoHead.rpcId);
				if (!document.Get("head", jsonValue))
				{
					return tcp::read::decode_error;
				}
				std::string value;
				for (const char* key : jsonValue.GetAllKey())
				{
					value.clear();
					if (!jsonValue.Get(key, value))
					{
						return tcp::read::decode_error;
					}
					this->mHead.Add(key, value);
				}
				if (document.Get("data", jsonValue))
				{
					size_t count = 0;
					std::unique_ptr<char> json;
					if (jsonValue.ToCString(json, count))
					{
						this->mBody.assign(json.get(), count);
					}
					return 0;
				}
				document.Get("data", this->mBody);
				break;
			}
		case rpc::msg::opcode:
			{
				unsigned short opcode = 0;
				tcp::Data::Read(os, opcode);
				this->mProtoHead.porto = rpc::proto::pb;
				this->mProtoHead.type = rpc::type::request;
				this->mProtoHead.Len = size - sizeof(opcode);
				{
					this->mProtoHead.rpcId = opcode;
					char* buffer = const_cast<char*>(this->mBody.c_str());
					os.readsome(buffer, this->mProtoHead.Len);
				}
				break;
			}
		}
		return 0;
	}

	int Message::OnSendMessage(std::ostream& os)
	{
		switch (this->mMsg)
		{
		case rpc::msg::bin:
			{
				this->mProtoHead.Len = this->mHead.GetLength() + this->mBody.size();
				tcp::Data::WriteHead(os, this->mProtoHead, true);

				this->mHead.OnSendMessage(os);
				os.write(this->mBody.c_str(), (int)this->mBody.size());
				break;
			}
		case rpc::msg::text:
			{
				tcp::Data::WriteHead(os, this->mProtoHead, false);
				this->mHead.OnSendMessage(os);
				os.write(this->mBody.c_str(), (int)this->mBody.size());
				break;
			}
		case rpc::msg::json:
			{
				json::w::Document document;
				document.Add("type", this->mProtoHead.type);
				document.Add("proto", this->mProtoHead.porto);
				document.Add("rpc_id", this->mProtoHead.rpcId);
				std::unique_ptr<json::w::Value> headObject = document.AddObject("head");
				for (const std::pair<std::string, std::string>& iter : this->mHead.GetValue())
				{
					headObject->Add(iter.first.c_str(), iter.second);
				}
				if (this->mProtoHead.porto == rpc::proto::json)
				{
					document.AddObject("data", this->mBody);
				}
				else
				{
					document.Add("data", this->mBody);
				}
				wrap::string<true> json;
				if (document.Serialize(json))
				{
					os.write(json.c_str(), json.size());
				}
				break;
			}
		case rpc::msg::opcode:
			{
				unsigned short opcode = this->GetOpcode();
				this->mProtoHead.Len = RPC_PACKET_LEN_BYTES
					+ sizeof(opcode) + this->mBody.size();
				
				os.write((char*)this->mProtoHead.Len, RPC_PACKET_LEN_BYTES);
				os.write((char*)opcode, sizeof(opcode));
				tcp::Data::Write(os, this->mBody.c_str(), this->mBody.size());
				break;
			}
		}

		return 0;
	}

	void Message::SetContent(const std::string& content)
	{
		this->mBody = content;
		this->mProtoHead.porto = rpc::proto::string;
	}

	void Message::SetContent(const char* content, size_t count)
	{
		this->mBody.assign(content, count);
		this->mProtoHead.porto = rpc::proto::string;
	}

	void Message::SetError(const std::string& content)
	{
		this->mBody = content;
		this->mProtoHead.porto = rpc::proto::error;
	}


	void Message::SetContent(const json::w::Document& content)
	{
		content.Serialize(&this->mBody);
		this->mProtoHead.porto = rpc::proto::json;
	}

	void Message::SetContent(char proto, const std::string& content)
	{
		this->mBody = content;
		this->mProtoHead.porto = proto;
	}

	void Message::SetContent(char proto, const char* content, size_t count)
	{
		this->mProtoHead.porto = proto;
		this->mBody.assign(content, count);
	}

	std::unique_ptr<Message> Message::Clone() const
	{
		std::unique_ptr<Message> message = std::make_unique<Message>();
		{
			message->SetNet(this->mNet);
			message->SetMsg(this->mMsg);
			for (const std::pair<std::string, std::string>& iter : this->mHead.GetValue())
			{
				message->mHead.Add(iter.first, iter.second);
			}
			message->mBody = this->mBody;
			message->mProtoHead = this->mProtoHead;
		}
		return message;
	}

	void Message::Clear()
	{
		this->mSockId = 0;
		this->mHead.Clear();
		this->mBody.clear();
		this->mNet = rpc::net::tcp;
		memset(&this->mProtoHead, 0, sizeof(this->mProtoHead));
	}

	std::string Message::ToString()
	{
		json::w::Document jsonWriter;
		jsonWriter.Add("type", this->mProtoHead.type);
		jsonWriter.Add("proto", this->mProtoHead.porto);
		jsonWriter.Add("source", this->mProtoHead.source);
		jsonWriter.Add("rpc_id", this->mProtoHead.rpcId);

		jsonWriter.Add("from", this->mSockId);
		auto data = jsonWriter.AddObject("head");
		for (const std::pair<std::string, std::string>& iter : this->mHead.GetValue())
		{
			data->Add(iter.first.c_str(), iter.second);
		}
		if (!this->mBody.empty())
		{
			jsonWriter.Add("data", this->mBody);
		}
		return jsonWriter.JsonString();
	}

	Message::Message() noexcept
	{
		this->mSockId = 0;
		this->mBody.clear();
		this->mHead.Clear();
		this->mNet = rpc::net::tcp;
		this->mMsg = rpc::msg::bin;
		this->mTimeout = 0;
		memset(&this->mProtoHead, 0, sizeof(this->mProtoHead));
	}

	Message::~Message() noexcept
	{
		this->Clear();
	}
}
