//
// Created by 64658 on 2025/2/18.
//

#ifndef APP_PGSQLCOMMON_H
#define APP_PGSQLCOMMON_H
#include <list>
#include "DB/Common/Url.h"
#include "DB/Common/Explain.h"
#include "Proto/Message/IProto.h"

#ifdef __SHARE_PTR_COUNTER__
#include "Core/Memory/MemoryObject.h"
#endif
namespace pgsql
{
	constexpr unsigned int VERSION = 196608;
	struct Cluster : public json::Object<Cluster>
	{
	public:
		int ping = 15;
		int count = 1;
		int retry = 5;
		bool debug = false;
		db::Explain explain;
		int conn_count = 3;
		std::string table;
		std::vector<std::string> address;
	public:
		inline static void RegisterAllFields()
		{
			REGISTER_JSON_CLASS_FIELD(pgsql::Cluster, ping);
			REGISTER_JSON_CLASS_FIELD(pgsql::Cluster, count);
			REGISTER_JSON_CLASS_FIELD(pgsql::Cluster, retry);
			REGISTER_JSON_CLASS_FIELD(pgsql::Cluster, debug);
			REGISTER_JSON_CLASS_FIELD(pgsql::Cluster, table);
			REGISTER_JSON_CLASS_FIELD(pgsql::Cluster, explain);
			REGISTER_JSON_CLASS_FIELD(pgsql::Cluster, conn_count);
			REGISTER_JSON_CLASS_MUST_FIELD(pgsql::Cluster, address);
		}
	};

	struct Config : public db::Url
	{
	public:
		Config() : db::Url("pgsql") { }
	public:
		std::string db;
		std::string user;
		int conn_count = 3;
		std::string table;
		std::string address;
		std::string password;
	};

	namespace type
	{
		constexpr char status = 'S';
		constexpr char auth = 'R';
		constexpr char backend_key = 'K';
		constexpr char ready_for_query = 'Z';
		constexpr char query = 'Q';
		constexpr char notice = 'N';
		constexpr char notification = 'A';
		constexpr char password = 'p';
		constexpr char row_description = 'T';
		constexpr char data_row = 'D';
		constexpr char command_complete = 'C';
		constexpr char error = 'E';
	}

	namespace field
	{
		constexpr unsigned int BOOL = 16; //布尔类型，值为 true 或 false
		constexpr unsigned int NUMBER_INT16 = 21; //16 位有符号整数
		constexpr unsigned int NUMBER_INT32 = 23; //32 位有符号整数
		constexpr unsigned int NUMBER_INT64 = 20; //64 位有符号整数
		constexpr unsigned int NUMBER_SERIAL = 2179; //自增整数
		constexpr unsigned int NUMBER_SMALL_SERIAL = 2180; //自增小整数
		constexpr unsigned int NUMBER_BIG_SERIAL = 2182; //自增大整数


		constexpr unsigned int NUMBER_FLOAT32 = 700; //单精度浮点数
		constexpr unsigned int NUMBER_FLOAT64 = 701; //双精度浮点数
		constexpr unsigned int NUMBER_NUMERIC = 1700; //任意精度的定点数

		constexpr unsigned int STRING_CHAR = 1042; //固定长度的字符串
		constexpr unsigned int STRING_VARCHAR = 1043; //可变长度的字符串，有长度限制
		constexpr unsigned int STRING_TEXT = 25; //可变长度的字符串，无长度限制
		constexpr unsigned int STRING_BYTES = 17; //二进制

		constexpr unsigned int STRING_JSON = 114; //json
		constexpr unsigned int STRING_JSONB = 3802; //jsonb

		constexpr unsigned int ARRAY_INT2 = 1005; //int2[]
		constexpr unsigned int ARRAY_INT4 = 1007; //int4[]
		constexpr unsigned int ARRAY_INT8 = 1016; //int4[]

		constexpr unsigned int ARRAY_TEXT = 1009;
		constexpr unsigned int ARRAY_VARCHAR = 1015;

		constexpr unsigned int ARRAY_FLOAT = 1021;
		constexpr unsigned int ARRAY_DOUBLE = 1022;

		constexpr unsigned int ARRAY_ANY = 2277;

	}

	namespace auth
	{
		constexpr unsigned int OK = 0;
		constexpr unsigned int PASSWORD = 3;  //明文密码
		constexpr unsigned int MD5_PASSWORD = 5; //md5密码
		constexpr unsigned int SCRAM_SHA_256 = 10; //sha256
	}

	struct FieldInfo
	{
		int type;
		std::string name;
	};
}

namespace pgsql
{
	inline unsigned int ReadLength(const char * buffer)
	{
		unsigned int length = (static_cast<unsigned char>(buffer[0]) << 24) |
							  (static_cast<unsigned char>(buffer[1]) << 16) |
							  (static_cast<unsigned char>(buffer[2]) << 8) |
							  static_cast<unsigned char>(buffer[3]);
		return length;
	}

	class Head
	{
	public:
		char type;
		unsigned int length;
	public:
		inline void Decode(const char * buffer)
		{
			this->type = buffer[0];
			this->length = ReadLength(buffer + 1);
		}
		inline void Decode(std::istream &os)
		{
			char buffer[5] = { 0};
			os.readsome(buffer, sizeof(buffer));
			this->Decode(buffer);
		}
	};

	class StartRequest : public tcp::IProto
	{
	public:
		StartRequest() = default;
	public:
		void Clear() final;
		int OnSendMessage(std::ostream &os) final;
	public:
		std::string db;
		std::string user;
	};

	struct ServerInfo
	{
	public:
		std::string timeZone;
		std::string serverVersion;
		std::string clientEncoding;
		std::string serverEncoding;
	};
}

namespace pgsql
{
	class Request : public tcp::IProto
#ifdef __SHARE_PTR_COUNTER__
		, public memory::Object<Request>
#endif
	{
	public:
		explicit Request();
		explicit Request(const std::string& sql);
		explicit Request(const char * sql, size_t  size);
		Request(unsigned char t, const std::string& message);
	public:
		void Clear() final;
		bool GetCommand(std::string & cmd) const;
		int OnSendMessage(std::ostream &os) final;
		void SetRpcId(int id) { this->mRpcId = id; }
		inline int GetRpcId() const { return this->mRpcId; }
		std::string ToString() final { return this->mSql.back(); }
	public:
		inline void EnableCommit() { this->mEnableCommit = true; } //开启事务执行
		inline bool IsEnableCommit() const { return this->mEnableCommit; }
		inline void AddBatch(const std::string & sql) { this->mSql.emplace_back(sql); }
		inline void AddBatch(const char * sql, size_t size) { this->mSql.emplace_back(sql, size); }
		inline size_t Count() const { return this->mEnableCommit ? this->mSql.size() + 1 : this->mSql.size(); }
	private:
		void WriteCommand(std::ostream &os, const std::string & sql) const;
	private:
		int mRpcId;
		unsigned char mType;
		bool mEnableCommit = false;
		std::list<std::string> mSql;
	};
}

namespace pgsql
{
	class Response
#ifdef __SHARE_PTR_COUNTER__
	: public memory::Object<Response>
#endif
	{
	public:
		int count = 0;
		int okCount = 0;
		std::string cmd;
		std::list<std::string> error;
		std::list<std::string> results;
	public:
		inline bool IsOk() const { return this->error.empty(); }
	};
}

namespace pgsql
{
	class Result : public tcp::IProto
#ifdef __SHARE_PTR_COUNTER__
		, public memory::Object<Result>
#endif
	{
	public:
		Result();
	public:
		void Clear() final;
		int OnSendMessage(std::ostream &os) final;
		int OnRecvMessage(std::istream &os, size_t size) final;
	public:
		void DecodeData(Response & response);
		bool GetStatus(const std::string & key, std::string & value);
	public:
		char mType;
		std::string mError;
		std::string mBuffer;
		std::string mResult;
		std::unordered_map<std::string, std::string> mStatus;
	private:
		int mDecodeStatus;
		std::vector<std::string> mRows;
		std::vector<pgsql::FieldInfo> mFields;
	};
}



#endif //APP_PGSQLCOMMON_H
