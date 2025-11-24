//
// Created by yy on 2025/2/10.
//

#include "MysqlProto.h"

#include <regex>
#include "Util/Crypt/sha1.h"
#include "XCode/XCode.h"
#include "Util/Tools/String.h"
#include "Yyjson/Document/Document.h"

#ifdef __ENABLE_OPEN_SSL__

#include "openssl/sha.h"

#endif

namespace mysql
{
	Request::Request()
	{
		this->mRpcId = 0;
		this->mIndex = 0;
		this->mCmd = mysql::cmd::QUERY;
	}

	Request::Request(char cmd)
			: mCmd(cmd)
	{
		this->mRpcId = 0;
		this->mIndex = 0;
	}

	Request::Request(const std::string& sql)
			: mCmd(mysql::cmd::QUERY)
	{
		this->mRpcId = 0;
		this->mIndex = 0;
		this->mSql.emplace_back(sql);
	}

	Request::Request(const char* sql, size_t size)
			:  mCmd(mysql::cmd::QUERY)
	{
		this->mRpcId = 0;
		this->mIndex = 0;
		this->mSql.emplace_back(sql, size);
	}

	Request::Request(char cmd, const std::string& sql)
			: mCmd(cmd)
	{
		this->mRpcId = 0;
		this->mIndex = 0;
		this->mSql.emplace_back(sql);
	}

	bool Request::GetCommand(std::string& cmd) const
	{
		if(this->mSql.empty())
		{
			return false;
		}
		size_t pos = this->mSql.back().find(' ');
		if (pos == std::string::npos)
		{
			return false;
		}
		cmd = this->mSql.back().substr(0, pos);
		help::Str::Toupper(cmd);
		return true;
	}

	std::string Request::ToString()
	{
		if(this->mSql.empty())
		{
			return "";
		}
		return this->mSql.back();
	}

	size_t Request::Count()
	{
		if(this->mSql.empty())
		{
			return 1;
		}
		if(!this->mCommit)
		{
			return this->mSql.size();
		}
		return this->mSql.size() + 1;
	}

	int Request::OnSendMessage(std::ostream& os)
	{
		if(this->mCommit)
		{
			const static std::string begin("START TRANSACTION");
			Request::WriteCommand(os, begin);
		}
		if(this->mSql.empty())
		{
			std::string empty;
			Request::WriteCommand(os, empty);
			return 0;
		}
		for(const std::string & sql : this->mSql)
		{
			Request::WriteCommand(os, sql);
		}
		return 0;
	}

	void Request::WriteCommand(std::ostream& os, const std::string& sql) const
	{
		uint32_t length = sql.size() + 1;
		std::array<uint8_t, 3> length_bytes = mysql::encode(length);
		os.write((char*)length_bytes.data(), (int)length_bytes.size());
		os << this->mIndex << this->mCmd;

		os.write(sql.c_str(), (int)sql.size());
	}

	void Request::Clear()
	{
		this->mCmd = 0;
		this->mSql.clear();
	}
}

namespace mysql
{
	namespace plugin
	{
		std::string native_password(const std::string& password, const std::string& salt)
		{
			std::string password_hash = help::Sha1::GetHash(password);
			std::string stage1 = help::Sha1::GetHash(password_hash);
			std::string salt_stage1 = salt + stage1;
			std::string challenge_response = help::Sha1::GetHash(salt_stage1);

			for (size_t i = 0; i < password_hash.size(); ++i)
			{
				challenge_response[i] ^= password_hash[i];
			}

			return challenge_response;
		}

#ifdef __ENABLE_OPEN_SSL__

		std::string caching_sha2_password(const std::string& password, const std::string& salt)
		{
			unsigned char hash1[SHA256_DIGEST_LENGTH];
			SHA256((const unsigned char*)password.data(), password.size(), hash1);

			// 步骤2：计算 SHA256(SHA256(password))
			unsigned char hash2[SHA256_DIGEST_LENGTH];
			SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);

			// 步骤3：拼接 SHA256(SHA256(password)) 和 salt
			std::vector<unsigned char> concat;
			concat.insert(concat.end(), hash2, hash2 + SHA256_DIGEST_LENGTH);
			concat.insert(concat.end(), salt.begin(), salt.end());

			// 步骤4：计算 SHA256(SHA256(SHA256(password)) + salt)
			unsigned char hash3[SHA256_DIGEST_LENGTH];
			SHA256(concat.data(), concat.size(), hash3);

			// 步骤5：计算最终结果：hash1 ^ hash3
			unsigned char result[SHA256_DIGEST_LENGTH];
			for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
				result[i] = hash1[i] ^ hash3[i];
			}

			// 返回二进制数据的安全表示
			return std::string((const char*)result, SHA256_DIGEST_LENGTH);
		}

#endif
	}
}

namespace mysql
{
	bool HandshakeResponse::OnDecode(mysql::Response & response)
	{
		unsigned int offset = 0;
		// 解析协议版本
		this->protocol_version = response.ReadChar(offset);
		const std::string & packet = response.GetBuffer();
		// 解析服务器版本（null 结尾字符串）
		size_t server_version_end = std::find(packet.begin() + 1, packet.end(), '\0') - packet.begin();
		this->server_version = std::string(packet.begin() + 1, packet.begin() + server_version_end);
		offset = server_version_end + 1;

		// 解析连接 ID
		this->connection_id = *reinterpret_cast<const uint32_t*>(&packet[offset]);
		offset += 4;

		// 解析盐值第一部分（8 字节）
		std::string salt_part1(packet.begin() + offset, packet.begin() + offset + 8);
		offset += 8;

		// 跳过填充值（1 字节）
		offset += 1;

		// 解析服务器能力标志（低 16 位）
		uint16_t capability_flags_low = *reinterpret_cast<const uint16_t*>(&packet[offset]);
		offset += 2;

		// 解析字符集
		this->character_set = packet[offset];
		offset += 1;

		// 解析服务器状态
		this->status_flags = *reinterpret_cast<const uint16_t*>(&packet[offset]);
		offset += 2;

		// 解析服务器能力标志（高 16 位）
		uint16_t capability_flags_high = *reinterpret_cast<const uint16_t*>(&packet[offset]);
		offset += 2;
		this->capability_flags = (capability_flags_high << 16) | capability_flags_low;

		// 解析认证插件数据长度
		uint8_t auth_plugin_data_len = packet[offset++];

		// 跳过保留字段（10 字节）
		offset += 10;

		// 解析盐值第二部分（12 字节）
		std::string salt_part2;
		if(this->capability_flags & mysql::client_flag::CLIENT_SECURE_CONNECTION)
		{
			salt_part2.assign(packet.begin() + offset, packet.begin() + offset + 12);
			offset += 12;
		}

		this->salt = salt_part1 + salt_part2;
		if(auth_plugin_data_len > 0 && (this->capability_flags & mysql::client_flag::CLIENT_PLUGIN_AUTH))
		{
			this->auth_plugin_name = std::string(packet.c_str() + offset + 1, auth_plugin_data_len);
		}
		return true;
	}
}

namespace mysql
{
	int LoginRequest::Encode(std::vector<uint8_t>& packet) const
	{
		uint32_t client_capabilities =
				mysql::client_flag::CLIENT_LONG_PASSWORD |
				mysql::client_flag::CLIENT_LONG_FLAG |
				mysql::client_flag::CLIENT_FOUND_ROWS |
				mysql::client_flag::CLIENT_PROTOCOL_41 |
				mysql::client_flag::CLIENT_MULTI_RESULTS |
				mysql::client_flag::CLIENT_LOCAL_FILES |
				mysql::client_flag::CLIENT_MULTI_STATEMENTS |
				mysql::client_flag::CLIENT_SECURE_CONNECTION |
				mysql::client_flag::CLIENT_TRANSACTIONS |
				mysql::client_flag::CLIENT_PS_MULTI_RESULTS |
				mysql::client_flag::CLIENT_DEPRECATE_EOF;


		std::string encrypted_password;
		if (!this->password.empty())
		{
			if (this->authPlugin == mysql::plugin::MYSQL_CLEAR_PASSWORD)
			{
				encrypted_password = this->password;
				encrypted_password.push_back('\0');
			}
			else if (this->authPlugin == mysql::plugin::MYSQL_NATIVE_PASSWORD)
			{
				encrypted_password = mysql::plugin::native_password(this->password, this->salt);
			}
//#ifdef __ENABLE_OPEN_SSL__
//			else if (this->authPlugin == mysql::plugin::CACHING_SHA2_PASSWORD)
//			{
//				//client_capabilities |= mysql::client_flag::CLIENT_SSL;
//				client_capabilities |= mysql::client_flag::CLIENT_PLUGIN_AUTH;
//				encrypted_password = mysql::plugin::caching_sha2_password(this->password, this->salt);
//			}
//#endif
			else
			{
				return XCode::AuthPluginNonsupport;
			}
		}

		if (!this->database.empty())
		{
			client_capabilities |= mysql::client_flag::CLIENT_CONNECT_WITH_DB;
		}
		//client_capabilities = 260047;
		packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&client_capabilities),
				reinterpret_cast<uint8_t*>(&client_capabilities) + 4);


		uint32_t max_packet_size = mysql::config::PACKAGE_MAX_SIZE;
		packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&max_packet_size),
				reinterpret_cast<uint8_t*>(&max_packet_size) + 4);

		packet.emplace_back(this->charset);

		packet.insert(packet.end(), 23, 0);
		packet.insert(packet.end(), this->user.begin(), this->user.end());
		packet.emplace_back(0); // 用户名以 null 结尾
		if (!encrypted_password.empty())
		{
			packet.emplace_back(encrypted_password.size()); // 插入密码长度
			packet.insert(packet.end(), encrypted_password.begin(), encrypted_password.end());
		}
		else
		{
			packet.emplace_back(0); // 空密码情况
		}

		if (!this->database.empty())
		{
			packet.insert(packet.end(), this->database.begin(), this->database.end());
			packet.emplace_back(0); // 以 null 结尾
		}

		packet.insert(packet.end(), this->authPlugin.begin(), this->authPlugin.end());
		packet.emplace_back(0); // 以 null 结尾
		return XCode::Ok;
	}

}

namespace mysql
{
	Response::Response()
	{
		this->Clear();
	}

	Response::Response(unsigned char code)
	{
		this->Clear();
		this->mPackageCode = code;
	}

	void Response::Clear()
	{
		this->mIndex = 0;
		this->mLength = 0;
		this->mErrorCode = 0;
		this->mPackageCode = 0;
		this->mMessage.clear();
		this->contents.clear();
		this->mDecodeStatus = tcp::Decode::None;
	}

	int Response::OnRecvMessage(std::istream& os, size_t size)
	{
		switch (this->mDecodeStatus)
		{
			case tcp::Decode::None:
			{
				if (size < 4)
				{
					return 4;
				}
				this->mLength = 0;
				char buffer[4] = { 0 };
				if (os.readsome((char*)buffer, sizeof(buffer)) != sizeof(buffer))
				{
					return tcp::read::decode_error;
				}
				this->mIndex = (unsigned char)buffer[3];
				this->mDecodeStatus = tcp::Decode::MessageHead;
				tcp::Data::Read((char*)buffer, this->mLength, 3, false);
				return this->mLength;
			}
			case tcp::Decode::MessageHead:
			{
				std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
				{
					os.readsome(buffer.get(), size);
				}
				return this->OnMessage(buffer.get(), size);
			}
			case tcp::Decode::MessageBody:
			{
				this->mMessage.resize(size);
				os.readsome((char*)this->mMessage.c_str(), size);
				break;
			}
			default:
				return tcp::read::decode_error;
		}
		return 0;
	}

	std::string Response::ToString() const
	{
		json::w::Document document;
		if (this->IsOk())
		{
			document.Add("code", "ok");
			document.Add("affected_rows", this->ok.mAffectedRows);
			document.Add("server_status", this->ok.mServerStatus);
			document.Add("warning_count", this->ok.mWarningCount);
			document.Add("last_insert_id", this->ok.mLastInsertId);
			if (!this->contents.empty())
			{
				std::unique_ptr<json::w::Value> jsonValue = document.AddArray("result");
				for (const std::string& json: this->contents)
				{
					jsonValue->AddObject(json.c_str(), json.size());
				}
			}
		}
		else
		{
			document.Add("code", "error");
			document.Add("error", this->mMessage);
		}
		return document.JsonString();
	}

	unsigned int Response::DecodeColumnCount(unsigned int& pos)
	{
		if (pos > this->mMessage.size())
		{
			return 0;
		}
		unsigned int offset = 0;
		unsigned char len = this->mMessage[pos++];
		switch (len)
		{
			case 0XFC:
				offset = 2;
				break;
			case 0XFD:
				offset = 4;
				break;
			case 0XFE:
				offset = 8;
				break;
			default:
			{
				if (len < 0XFB)
				{
					return len;
				}
			}
		}
		if (pos + offset >= this->mMessage.size())
		{
			return 0;
		}
		unsigned int length = len;
		tcp::Data::Read(this->mMessage.c_str() + pos, length, offset, false);
		pos += offset;
		return length;
	}

	void Response::SkipString(unsigned int& pos)
	{
		unsigned int len = this->DecodeColumnCount(pos);
		if (len <= 0 || len + pos > this->mMessage.size())
		{
			return;
		}
		pos += len;
	}

	std::string Response::ReadString(unsigned int& pos)
	{
		std::string result;
		unsigned int len = this->DecodeColumnCount(pos);
		if (len <= 0 || len + pos > this->mMessage.size())
		{
			return result;
		}
		result.assign(this->mMessage.c_str() + pos, len);
		pos += len;
		return result;
	}

	std::string Response::ReadCString(unsigned int& pos)
	{
		std::string result;
		for(unsigned int index = pos; index < this->mMessage.size(); index++)
		{
			if(this->mMessage[index] == '\0')
			{
				break;
			}
			result += this->mMessage[index];
		}
		pos += result.size();
		return result;
	}

	void Response::ReadField(unsigned int& pos, mysql::FieldInfo& fieldInfo)
	{
		this->SkipString(pos); // std::string catalog = this->ReadString(pos);
		this->SkipString(pos); // std::string schema = this->ReadString(pos);
		this->SkipString(pos); // std::string table = this->ReadString(pos);
		this->SkipString(pos); // std::string org_table = this->ReadString(pos);
		fieldInfo.name = this->ReadString(pos);
		this->SkipString(pos); //std::string org_name = this->ReadString(pos);
		pos += 7;
		fieldInfo.type = this->ReadChar(pos);
		unsigned short flags = this->ReadShort(pos);
		unsigned char decimals = this->ReadChar(pos);
	}

	unsigned char Response::ReadChar(unsigned int& pos)
	{
		unsigned char result = 0;
		if (pos < this->mMessage.size())
		{
			result = this->mMessage[pos];
			pos++;
		}
		return result;
	}

	unsigned short Response::ReadShort(unsigned int& pos)
	{
		unsigned short result = 0;
		if (pos < this->mMessage.size())
		{
			tcp::Data::Read(this->mMessage.data() + pos, result, 2, false);
			pos += 2;
		}
		return result;
	}

	unsigned int Response::ReadInt(unsigned int& pos)
	{
		unsigned int result = 0;
		if (pos < this->mMessage.size())
		{
			tcp::Data::Read(this->mMessage.data() + pos, result, sizeof(result), false);
			pos += sizeof(result);
		}
		return result;
	}

	int Response::OnMessage(const char* buffer, size_t size)
	{
		unsigned int offset = 0;
		this->mPackageCode = (unsigned char)buffer[offset++];
		switch (this->mPackageCode)
		{
			case mysql::PACKAGE_OK:
			{
				this->ParseOkResponse(buffer + offset, size - offset);
				return tcp::read::done;
			}
			case mysql::PACKAGE_ERR:
			{
				tcp::Data::Read(buffer + offset, this->mErrorCode);
				offset += sizeof(this->mErrorCode);

				offset += 6;
				this->mMessage.assign(buffer + offset, size - offset);
				return tcp::read::done;
			}
			case mysql::PACKAGE_EOF:
			{
				unsigned short warnCount = 0;
				unsigned short statusFlags = 0;
				tcp::Data::Read(buffer + offset, warnCount, 2, false);
				offset += sizeof(warnCount);
				tcp::Data::Read(buffer + offset, statusFlags, 2, false);
				return tcp::read::done;
			}
			default:
				this->mMessage.assign(buffer, size);
				return tcp::read::pause;
		}
	}

	void Response::ParseOkResponse(const char* buffer, size_t size)
	{
		unsigned int offset = 0;
		if (size >= 6)
		{
			this->ok.mAffectedRows = (unsigned int)buffer[offset++];
			this->ok.mLastInsertId = (unsigned int)buffer[offset++];
			tcp::Data::Read(buffer + offset, this->ok.mServerStatus, 2, false);
			offset += 2;
			tcp::Data::Read(buffer + offset, this->ok.mWarningCount, 2, false);
			offset += 2;
			if (offset < size)
			{
				this->ok.mInfo.assign(buffer + offset, size - offset);
				offset = size;
			}
		}

		if (offset < size)
		{
			this->mMessage.assign(buffer + offset, size - offset);
		}
//		if((this->mOkResult.mServerStatus & SERVER_MORE_RESULTS_EXISTS) != 0)
//		{
//			return tcp::read::pause;
//		}
	}
}