#pragma once
#include "Sqlite/sqlite3.h"
#include "Core/Wrap/Wrap.h"
#include "DB/Common/TableInfo.h"
#include "DB/Common/SqlFactory.h"
#include "Proto/Include/Message.h"
#include "Util/Tools/NumberBuilder.h"
#include "Entity/Component/Component.h"
#include "Yyjson/Object/JsonObject.h"
#include "Util/File/FileFactory.h"
#include "Lua/Engine/LuaInclude.h"
namespace sqlite
{
	struct Config : public json::Object<Config>
	{
	public:
		std::string file;
		std::string mode;
		std::string table;
	};

	struct Response
	{
	public:
		bool ok = true;
		int count = 0;
		std::string error;
		std::list<std::unique_ptr<json::r::Document>> result;
	};

	class ArgsBinder
	{
	public:
		explicit ArgsBinder(sqlite3_stmt * s) : stmt(s) { }
	public:
		inline void Bind(const std::string & str)
		{
			this->Bind(str.c_str(), str.size());
		}
		inline void Bind(const char * str, size_t size)
		{
			sqlite3_bind_text(this->stmt, index++, str, size, SQLITE_TRANSIENT);
		}

		inline void Bind(const wrap::string<> & str)
		{
			sqlite3_bind_text(this->stmt, index++, str.c_str(), str.size(), SQLITE_TRANSIENT);
		}

		inline void Bind(const json::w::Document & jsonValue)
		{
			wrap::string<true> json;
			if(jsonValue.Serialize(json))
			{
				this->Bind(json.c_str(), json.size());
			}
		}

		template<typename T>
		inline std::enable_if_t<std::is_integral<T>::value, void> Bind(T value)
		{
			if(sizeof(T) == sizeof(long long))
			{
				sqlite3_bind_int64(this->stmt, index++, (long long)value);
				return;
			}
			sqlite3_bind_int(this->stmt, index++, (int)value);
		}
		template<typename T>
		inline std::enable_if_t<std::is_floating_point<T>::value, void> Bind(T value)
		{
			sqlite3_bind_double(this->stmt, index++, (double)value);
		}

		template<typename... Args>
		inline void BindArgs(Args... args)
		{
			ArgsBinder::Encode(this, std::forward<Args>(args)...);
		}
	private:
		static void Encode(ArgsBinder* self) { }
		template<typename T, typename... Args>
		static void Encode(ArgsBinder* self, const T& t, Args... args)
		{
			self->Bind(t);
			ArgsBinder::Encode(self, std::forward<Args>(args)...);
		}
	private:
		int index = 1;
		sqlite3_stmt * stmt = nullptr;
	};
}

namespace acs
{
	class SqliteComponent final : public Component, public IDestroy, public IRefresh, public IServerRecord
	{
	public:
		SqliteComponent();
	public:
		bool Insert(const char * tab, json::w::Value & document);
		bool Delete(const char * tab, json::w::Value & filter, int limit = 1);
		bool Update(const char * tab, json::w::Value & filter, json::w::Value & document);
		bool Query(const char * tab, json::w::Value & filter, std::vector<std::string> & result);
	public:
		bool StartTransaction(); //开始事务
		bool CommitTransaction(); //结束事务
		bool RollbackTransaction(); //回滚事务
	public:
		std::unique_ptr<sqlite::Response> Run(const std::string & sql);
		std::unique_ptr<sqlite::Response> Run(const char * sql, size_t size);
		std::unique_ptr<sqlite::Response> Invoke(const std::string & name, lua_State * L);
	public:
		bool Build(const std::string & name, const std::string & sql);
		bool Build(const std::string & name, const char* sql, size_t size);
		template<typename ... Args>
		inline std::unique_ptr<sqlite::Response> Invoke(const std::string & name, Args&& ...args)
		{
			auto iter = this->mStmtInfos.find(name);
			if(iter == this->mStmtInfos.end())
			{
				std::unique_ptr<sqlite::Response> response = std::make_unique<sqlite::Response>();
				{
					response->ok = false;
					response->error = "not find stmt name : " + name;
				}
				return response;
			}
			sqlite::ArgsBinder binder(iter->second);
			binder.BindArgs(std::forward<Args>(args)...);
			return this->Run(iter->second);
		}
	public:
		bool Del(const std::string & key);
		bool Get(const std::string & key, std::string & value);
		bool SetTimeout(const std::string & key, int timeout);
		bool Set(const std::string & key, const std::string & value);
		bool Get(const std::string & key, json::r::Document & value);
		bool Set(const std::string & key, const json::w::Document & value);
	private:
		bool RemoveTimeoutData();
		std::unique_ptr<sqlite::Response> Run(sqlite3_stmt * stmt);
		bool InitTable(const std::string & name, const sql::Table & tableInfo);
		bool CreateTable(const std::string & name, const sql::Table & tableInfo);
	private:
		bool Init();
		bool Awake() final;
		bool OnRefresh() final;
		void OnDestroy() final;
		void OnRecord(json::w::Document &document) final;
	private:
		unsigned int mSum;
		std::string mName;
		sqlite3 * mDatabase;
		sqlite::Config mConfig;
		sql::Factory mFactory;
		help::FileFactory mFileFactory;
		std::unordered_map<std::string, sqlite3_stmt *> mStmtInfos;
	};
}