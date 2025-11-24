//
// Created by yy on 2025/2/10.
//

#ifndef APP_MYSQLPROTO_H
#define APP_MYSQLPROTO_H

#include <list>
#include <array>
#include "Proto/Message/IProto.h"
#include "Mysql/Common/MysqlCommon.h"

#ifdef __SHARE_PTR_COUNTER__
#include "Core/Memory/MemoryObject.h"
#endif

namespace mysql
{
	inline int ReadLength(const char * header)
	{
		return header[0] | (header[1] << 8) | (header[2] << 16);
	}

	template<typename T = uint8_t>
	inline T read(const char * buff, size_t size = sizeof(T))
	{
		T value = 0;
		std::memcpy(&value, buff, size);
		return value;
	}
	inline std::array<uint8_t, 3> encode(uint32_t length)
	{
		return std::array<uint8_t, 3>{ static_cast<uint8_t>(length & 0xFF),
				 static_cast<uint8_t>((length >> 8) & 0xFF),
				 static_cast<uint8_t>((length >> 16) & 0xFF) };
	}
}

namespace mysql
{
	class Request : public tcp::IProto
#ifdef __SHARE_PTR_COUNTER__
		, public memory::Object<Request>
#endif
	{
	public:
		explicit Request();
		explicit Request(char cmd);
		explicit Request(const std::string&  message);
		explicit Request(const char * sql, size_t size);
		explicit Request(char cmd, const std::string& message);
	public:
		bool GetCommand(std::string & cmd) const;
		void SetRpcId(int id) { this->mRpcId = id; }
		int GetRpcId() const { return this->mRpcId; }
		inline char GetCmd() const { return this->mCmd; }
		void SetIndex(unsigned char idx) { this->mIndex = idx; }
	public:
		size_t Count();
		std::string ToString() final;
		inline void EnableCommit() { this->mCommit = true; } //开启事务执行
		inline bool IsEnableCommit() const { return this->mCommit; }
		inline const std::list<std::string> & GetList() const { return this->mSql; }
		inline void AddBatch(const std::string & sql) { this->mSql.emplace_back(sql); }
		inline void AddBatch(const char * sql, size_t size) { this->mSql.emplace_back(sql, size); }
	private:
		void Clear() final;
		int OnSendMessage(std::ostream& os) final;
		void WriteCommand(std::ostream & os, const std::string & sql) const;
	private:
		char mCmd;
		int mRpcId;
		bool mCommit = false; //是否事务执行
		unsigned char mIndex;
		std::list<std::string> mSql;
	};
}

namespace mysql
{
	class LoginRequest
	{
	public:
		std::string salt;
		std::string user;
		std::string database;
		std::string password;
		unsigned char charset;
		std::string authPlugin;
		bool enable_compression = false; // 新增：是否启用压缩
		int zstd_compression_level = 3;  // 新增：zstd 压缩级别，默认 3
	public:
		int Encode(std::vector<uint8_t> & packet) const;
	};
}


namespace mysql
{
	namespace plugin
	{
		//CREATE USER 'yy'@'localhost' IDENTIFIED WITH mysql_native_password BY '199595yjz.';
		//ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'password'; //修改插件认证方式
		constexpr const char * MYSQL_CLEAR_PASSWORD = "mysql_clear_password";
		constexpr const char * MYSQL_NATIVE_PASSWORD = "mysql_native_password";
		constexpr const char * CACHING_SHA2_PASSWORD = "caching_sha2_password";

		extern std::string native_password(const std::string & password, const std::string & salt);
#ifdef __ENABLE_OPEN_SSL__
		extern std::string caching_sha2_password(const std::string & password, const std::string & salt);
#endif
	}
}

namespace mysql
{
	class Response
#ifdef __SHARE_PTR_COUNTER__
	: public memory::Object<Request>
#endif
	{
	public:
		Response();
		explicit Response(unsigned char code);
	public:
		void Clear() ;
		int OnRecvMessage(std::istream &os, size_t size);
	public:
		std::string ToString() const;
		unsigned char ReadChar(unsigned int &pos);
		void SkipString(unsigned int & pos);
		std::string ReadString(unsigned int & pos);
		std::string ReadCString(unsigned int & pos);
		unsigned int ReadInt(unsigned int & pos);
		unsigned short ReadShort(unsigned int & pos);
		unsigned char GetIndex() const { return this->mIndex; }
		unsigned int DecodeColumnCount(unsigned int & pos);
		unsigned short GetErrorCode() const { return this->mErrorCode; }
		const std::string & GetBuffer() const { return this->mMessage;}
		void ReadField(unsigned int & pos, mysql::FieldInfo & fieldInfo);
		unsigned char GetPackageCode() const { return this->mPackageCode; }
	public:
		bool IsOk() const { return this->mPackageCode != mysql::PACKAGE_ERR; }
		bool IsEof() const { return this->mPackageCode == mysql::PACKAGE_EOF; }
		bool HasError() const { return this->mPackageCode == mysql::PACKAGE_ERR || !this->error.empty(); }
	private:
		int OnMessage(const char * buffer, size_t size);
		void ParseOkResponse(const char * buffer, size_t size);
	private:
		int mLength;
		int mDecodeStatus;
		std::string mMessage;
		unsigned char mIndex;
		unsigned short mErrorCode;
		unsigned char mPackageCode;
	public:
		mysql::OKResponse ok;
		std::list<std::string> error;
		std::list<std::string> contents;
	};
}

namespace mysql
{
	class HandshakeResponse
	{
	public:
		bool OnDecode(mysql::Response & response);
	public:
		uint8_t filler;
		uint8_t protocol_version;    // 协议版本
		std::string server_version;  // 服务器版本
		uint32_t connection_id;      // 连接 ID
		std::string salt;            // 盐值 (20 字节)
		uint32_t capability_flags;   // 服务器能力标志
		uint8_t character_set;       // 字符集
		uint16_t status_flags;       // 服务器状态
		std::string auth_plugin_name; // 认证插件名称
	};
}


#endif //APP_MYSQLPROTO_H
