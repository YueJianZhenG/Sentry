#include "SqliteComponent.h"
#include "Server/Config/ServerConfig.h"
#include "Util/File/DirectoryHelper.h"
#include "Lua/Lib/Lib.h"
#include "Util/Tools/TimeHelper.h"
#include "DB/Common/SqlFactory.h"
#include "Util/Tools/String.h"
#include "Timer/Timer/ElapsedTimer.h"
#include "Yyjson/Lua/ljson.h"
constexpr const char* LOCAL_DATA = "local_data";

#ifdef __ENABLE_MI_MALLOC__
#include "mimalloc.h"
namespace sqlite
{
	inline void* malloc(int size)
	{
		return mi_malloc(size);
	}

	inline void free(void* p)
	{
		mi_free(p);
	}

	inline void* realloc(void* p, int size)
	{
		return mi_realloc(p, size);
	}

	inline int mi_size(void* p)
	{
		return (int)mi_usable_size(p);
	}

	inline int mi_roundup(int size)
	{
		return size;
	}

	inline int m_init(void*)
	{
		return 0;
	}

	sqlite3_mem_methods methods = {
			malloc,
			free,
			realloc,
			mi_size,
			mi_roundup,
			m_init,
			nullptr,
			nullptr,
	};
}

#endif

namespace acs
{
    SqliteComponent::SqliteComponent()
        : mDatabase(nullptr)
    {
		this->mSum = 0;
    	sql::RegisterObject();
        REGISTER_JSON_CLASS_FIELD(sqlite::Config, table);
        REGISTER_JSON_CLASS_MUST_FIELD(sqlite::Config, file);
        REGISTER_JSON_CLASS_MUST_FIELD(sqlite::Config, mode);
    }

    bool SqliteComponent::Awake()
    {
        LuaCCModuleRegister::Add([](Lua::CCModule& ccModule) {
            ccModule.Open("db.sqlite", lua::lib::luaopen_lsqlitedb);
        });
        LOG_CHECK_RET_FALSE(ServerConfig::Inst()->Get("sqlite", this->mConfig));
        return this->Init();
    }

    bool SqliteComponent::Init()
    {
		const std::string & file = this->mConfig.file;
		if(this->mConfig.mode != "memory")
		{
			std::string dir;
			if(help::dir::GetDirByPath(file, dir) && !dir.empty())
			{
				help::dir::MakeDir(dir);
			}
		}
#ifdef __ENABLE_MI_MALLOC__
		if(sqlite3_config(SQLITE_CONFIG_MALLOC, &sqlite::methods) != SQLITE_OK)
		{
			return false;
		}
		if(sqlite3_initialize() != SQLITE_OK)
		{
			return false;
		}
#endif
		const std::string & mode = this->mConfig.mode;
		const std::string url = fmt::format("file:{}?mode={}", file, mode);
		int flag = SQLITE_OPEN_URI | SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE;
        if (sqlite3_open_v2(url.c_str(), &this->mDatabase, flag, nullptr) != SQLITE_OK)
        {
            LOG_ERROR("open sqlite [{}] => {}", url, sqlite3_errmsg(this->mDatabase));
            return false;
        }
		LOG_CHECK_RET_FALSE(this->OnRefresh());
		this->mFactory.GetTable(LOCAL_DATA).Begin().AddColumn("field").VarChar(64).Unique().NotNull().Next();
		this->mFactory.AddColumn("content").VarChar(2048).NotNull().Next();
		this->mFactory.AddColumn("exp_time").BigInt().NotNull().Default(0).Next();
		std::string createSql = this->mFactory.AddColumn("last_time").BigInt().NotNull().End().ToString();
		std::string setIndexSql = this->mFactory.GetTable(LOCAL_DATA).SetIndexNotExist("exp_time", false).ToString();

		LOG_CHECK_RET_FALSE(this->Run(createSql.c_str(), createSql.size())->ok);
		LOG_CHECK_RET_FALSE(this->Run(setIndexSql.c_str(), setIndexSql.size())->ok);
		this->Build("local_data_delete", fmt::format("DELETE FROM {} WHERE field=?", LOCAL_DATA));
		this->Build("local_data_timeout", fmt::format("UPDATE {} SET exp_time=? WHERE field=?", LOCAL_DATA));
		this->Build("local_data_update", fmt::format("UPDATE {} SET content=?,last_time=? WHERE field=?", LOCAL_DATA));
		this->Build("local_data_select", fmt::format("SELECT field,content,exp_time FROM {} WHERE field=?", LOCAL_DATA));
		this->Build("local_data_insert", fmt::format("INSERT INTO {} (field,content,last_time)VALUES(?,?,?)", LOCAL_DATA));
		return this->RemoveTimeoutData();
    }

	bool SqliteComponent::Build(const std::string& name, const std::string& sql)
	{
		return this->Build(name, sql.c_str(), sql.size());
	}

	bool SqliteComponent::Build(const std::string& name, const char* sql, size_t size)
	{
		sqlite3_stmt* stmt = nullptr;
		int code = sqlite3_prepare_v2(this->mDatabase, sql, size, &stmt, nullptr);
		if (code != SQLITE_OK)
		{
			LOG_ERROR("[build sql] {}", sql);
			LOG_ERROR("[error] {}", sqlite3_errmsg(this->mDatabase));
			return false;
		}
		auto iter = this->mStmtInfos.find(name);
		if(iter != this->mStmtInfos.end())
		{
			sqlite3_finalize(iter->second);
			this->mStmtInfos.erase(iter);
		}
		this->mStmtInfos.emplace(name, stmt);
		return true;
	}

	bool SqliteComponent::InitTable(const std::string& name, const sql::Table& tableInfo)
	{
		std::string sql = fmt::format("PRAGMA table_info({})", name);
		std::unique_ptr<sqlite::Response> response = this->Run(sql);
		if (!response->ok || response->result.empty())
		{
			if(!this->CreateTable(name, tableInfo))
			{
				return false;
			}
			LOG_INFO("create mysql table [{}] ok", name);
			return true;
		}
		std::unordered_map<std::string, std::string> fields;
		for(const std::unique_ptr<json::r::Document> & document1 : response->result)
		{
			std::string field, type;
			document1->Get("type", type);
			document1->Get("name", field);
			fields.emplace(field, help::Str::Toupper(type));
		}

		sql::Factory sqlFactory;
		for(auto iter = tableInfo.fields.begin(); iter != tableInfo.fields.end(); iter++)
		{
			const std::string & field = iter->first;
			if(fields.find(field) == fields.end())
			{
				sqlFactory.GetTable(name);
				const sql::Field & fieldInfo = iter->second;
				sqlFactory.AlterColumn(field.c_str(), fieldInfo.type);
				if(fieldInfo.notnull)
				{
					sqlFactory.NotNull();
				}
				sqlFactory.Default(fieldInfo.default_val);
				const std::string sql1 = sqlFactory.ToString();
				std::unique_ptr<sqlite::Response> sqliteResponse = this->Run(sql1);

				if(sqliteResponse->ok)
				{
					LOG_DEBUG("[{}] add column ({}) ok", name, field);
				}
				if(fieldInfo.index)
				{
					sqlFactory.GetTable(name).SetIndex(field, false);
					const std::string sql2 = sqlFactory.ToString();
					std::unique_ptr<sqlite::Response> mysqlResponse1 = this->Run(sql2);
					if(mysqlResponse1->ok)
					{
						LOG_DEBUG("[{}] set index ({}) ok", name, field);
					}
				}
			}
		}
		return true;
	}

	bool SqliteComponent::CreateTable(const std::string& name, const sql::Table& tableInfo)
	{
		sql::Factory sqlFactory;
		json::r::Value jsonFields;
		std::vector<std::string> indexes;
		std::vector<std::string> primary;
		sqlFactory.GetTable(name).Begin();
		for (auto iter = tableInfo.fields.begin(); iter != tableInfo.fields.end(); iter++)
		{
			const sql::Field& fieldInfo = iter->second;
			sqlFactory.AppendColumn(fieldInfo.name.c_str(), fieldInfo.type.c_str());
			if (fieldInfo.notnull)
			{
				sqlFactory.NotNull();
			}
			sqlFactory.Default(fieldInfo.default_val);
			if (fieldInfo.unique)
			{
				sqlFactory.Unique();
			}
			if (fieldInfo.index)
			{
				indexes.emplace_back(fieldInfo.name);
			}
			if (fieldInfo.primary_key)
			{
				primary.emplace_back(fieldInfo.name);
			}
			sqlFactory.Next();
		}
		sqlFactory.PrimaryKey(primary).End();
		const std::string sql = sqlFactory.ToString();
		std::unique_ptr<sqlite::Response> response = this->Run(sql.c_str(), sql.size());
		if(!response->ok)
		{
			return false;
		}
		LOG_INFO("create sqlite [{}] ok", name);
		for(const std::string & field : indexes)
		{
			sqlFactory.GetTable(name).SetIndex(field, false);
			const std::string sql1 = sqlFactory.ToString();
			std::unique_ptr<sqlite::Response> response1 = this->Run(sql1.c_str(), sql1.size());
			if(response1->ok)
			{
				LOG_INFO("[{}] create pgsql index [{}] ok", name, field);
			}
		}
		return true;
	}

    bool SqliteComponent::OnRefresh()
    {
		std::vector<std::string> filePaths;
		const std::string& dir = this->mConfig.table;
		if (help::dir::GetFilePaths(dir, ".sql", filePaths) > 0)
		{
			for (const std::string& path: filePaths)
			{
				if (!this->mFileFactory.IsChange(path))
				{
					continue;
				}
				std::string sql;
				if (!this->mFileFactory.Read(path, sql))
				{
					LOG_ERROR("read sql file => {}", path);
					return false;
				}
				this->Run(sql);
			}
			filePaths.clear();
		}
		if (help::dir::GetFilePaths(dir, ".json", filePaths) > 0)
		{
			for (const std::string& path: filePaths)
			{
				if(!this->mFileFactory.IsChange(path))
				{
					continue;
				}
				sql::Table mysqlTable;
				json::r::Document document;
				if (!this->mFileFactory.Read(path, document, mysqlTable))
				{
					LOG_ERROR("decode json fail => {}", path);
					return false;
				}
				std::vector<std::string> tables;
				tables.emplace_back(mysqlTable.name);
				if(mysqlTable.count > 0)
				{
					tables.clear();
					for (int index = 0; index < mysqlTable.count; index++)
					{
						int idx = index + 1;
						tables.emplace_back(fmt::format("{}{}", mysqlTable.name, idx));
					}
				}
				for (const std::string& tab: tables)
				{
					timer::ElapsedTimer timer1;
					if (this->InitTable(tab, mysqlTable))
					{
						LOG_BY_NAME("sqlite", "[{}ms] init sqlite table [{}] ok", timer1.GetMs(), tab);
					}
				}
			}
		}
		return true;
    }

	bool SqliteComponent::RemoveTimeoutData()
	{
    	long long nowTime = help::Time::NowSec();
    	std::string sql = fmt::format("DELETE FROM {} WHERE exp_time>={}", LOCAL_DATA, nowTime);
    	std::unique_ptr<sqlite::Response> response = this->Run(sql);
    	if(!response->ok)
    	{
    		LOG_ERROR("{}", response->error);
    		return false;
    	}
    	return true;
	}


    void SqliteComponent::OnDestroy()
    {
		auto iter = this->mStmtInfos.begin();
		for(; iter != this->mStmtInfos.end(); iter++)
		{
			sqlite3_finalize(iter->second);
		}
		this->mStmtInfos.clear();
        sqlite3_close(this->mDatabase);
    }

	void SqliteComponent::OnRecord(json::w::Document& document)
	{
		std::unique_ptr<json::w::Value> jsonValue = document.AddObject("sqlite");
		{
			jsonValue->Add("sum", this->mSum);
			jsonValue->Add("stmt", this->mStmtInfos.size());
		}
	}

	bool SqliteComponent::Insert(const char* tab, json::w::Value& document)
	{
		json::r::Document document1(document);
		this->mFactory.GetTable(tab).Insert(document1);
		const std::string sql = this->mFactory.ToString();
		return this->Run(sql.c_str(), sql.size())->ok;
	}

	bool SqliteComponent::Update(const char* tab, json::w::Value& filter, json::w::Value& document)
	{
		json::r::Document filter1(filter);
		json::r::Document document1(document);
		this->mFactory.GetTable(tab).Update(document1).Filter(filter1).Limit(1);
		const std::string sql = this->mFactory.ToString();
		return this->Run(sql.c_str(), sql.size())->ok;
	}

	bool SqliteComponent::Delete(const char* tab, json::w::Value& filter, int limit)
	{
		json::r::Document filter1(filter);
		this->mFactory.GetTable(tab).Delete().Filter(filter1).Limit(limit);
		const std::string sql = this->mFactory.ToString();
		return this->Run(sql.c_str(), sql.size())->ok;
	}

	std::unique_ptr<sqlite::Response> SqliteComponent::Run(const std::string& sql)
	{
		return this->Run(sql.c_str(), sql.size());
	}

    std::unique_ptr<sqlite::Response> SqliteComponent::Run(const char * sql, size_t size)
	{
		sqlite3_stmt* stmt = nullptr;
		std::unique_ptr<sqlite::Response> response;
		do
		{
			if (sqlite3_prepare_v2(this->mDatabase, sql, size, &stmt, nullptr) != SQLITE_OK)
			{
				response = std::make_unique<sqlite::Response>();
				{
					response->ok = false;
					response->error.assign(sqlite3_errmsg(this->mDatabase));
				}
				break;
			}
			response = this->Run(stmt);
		}
		while(false);
		if(stmt != nullptr)
		{
			sqlite3_finalize(stmt);
		}
		return response;
	}

	bool SqliteComponent::Query(const char* tab, json::w::Value& filter, std::vector<std::string>& result)
	{
		std::string error;
		json::r::Document document(filter);
		this->mFactory.GetTable(tab).Select().Filter(document);
		std::string sql = this->mFactory.ToString();
		return this->Run(sql.c_str(), sql.size())->ok;
	}

    bool SqliteComponent::Del(const std::string& key)
    {
		static std::string func("local_data_delete");
		return this->Invoke(func, key)->ok;
    }

    bool SqliteComponent::Get(const std::string& key, std::string& value)
    {
		static std::string func("local_data_select");
		std::unique_ptr<sqlite::Response> response = this->Invoke(func, key);
		if(!response->ok || response->result.empty())
		{
			return false;
		}
		std::unique_ptr<json::r::Document> & document = response->result.front();
		{
			long long expTime = 0;
			long long nowTime = help::Time::NowSec();
			LOG_CHECK_RET_FALSE(document->Get("exp_time", expTime))
			if(expTime > 0 && nowTime >= expTime)
			{
				this->Del(key);
				return false;
			}
		}
		return document->Get("content", value);
    }

	bool SqliteComponent::Get(const std::string& key, json::r::Document& value)
	{
		static std::string func("local_data_select");
		std::unique_ptr<sqlite::Response> response = this->Invoke(func, key);
		if(!response->ok || response->result.empty())
		{
			return false;
		}
		std::unique_ptr<json::r::Document> & document = response->result.front();
		{
			long long expTime = 0;
			long long nowTime = help::Time::NowSec();
			LOG_CHECK_RET_FALSE(document->Get("exp_time", expTime))
			if(expTime > 0 && nowTime >= expTime)
			{
				this->Del(key);
				return false;
			}
		}
		size_t count = 0;
		const char * str = document->GetString("content", count);
		if(str == nullptr || count == 0)
		{
			return false;
		}
		return value.Decode(str, count);
	}

    bool SqliteComponent::Set(const std::string& key, const std::string& value)
    {
		long long nowTime = help::Time::NowSec();
		static std::string func("local_data_update");
		std::unique_ptr<sqlite::Response> response = this->Invoke(func, value, nowTime, key);
		if(!response->ok || response->count == 0)
		{
			response = this->Invoke("local_data_insert", key, value, nowTime);
		}
		return response->ok && response->count >= 1;
    }

	bool SqliteComponent::Set(const std::string& key, const json::w::Document& value)
	{
		std::string data;
		if(!value.Serialize(&data))
		{
			return false;
		}
		return this->Set(key, data);
	}

    bool SqliteComponent::SetTimeout(const std::string& key, int timeout)
    {
		static std::string func("local_data_timeout");
        long long expTime = help::Time::NowSec() + timeout;
		return this->Invoke(func, expTime, key)->ok;
    }

	std::unique_ptr<sqlite::Response> SqliteComponent::Run(sqlite3_stmt* stmt)
	{
		this->mSum++;
		int code = sqlite3_step(stmt);
		std::unique_ptr<sqlite::Response> response = std::make_unique<sqlite::Response>();
		{
			if(code < SQLITE_ROW && code != SQLITE_OK)
			{
				response->ok = false;
				response->error = sqlite3_errmsg(this->mDatabase);
				LOG_ERROR("{}", response->error)
				return response;
			}
			response->count = sqlite3_changes(this->mDatabase);
			while (code == SQLITE_ROW)
			{
				json::w::Document document;
				int count = sqlite3_column_count(stmt);
				for (int index = 0; index < count; index++)
				{
					const char* key = sqlite3_column_name(stmt, index);
					switch (sqlite3_column_type(stmt, index))
					{
						case SQLITE_INTEGER:
						{
							long long value = sqlite3_column_int64(stmt, index);
							document.Add(key, value);
							break;
						}
						case SQLITE_FLOAT:
						{
							double value = sqlite3_column_double(stmt, index);
							document.Add(key, value);
							break;
						}
						case SQLITE_TEXT:
						{
							const char* value = (char*)sqlite3_column_text(stmt, index);
							const int length = sqlite3_column_bytes(stmt, index);
							if (value != nullptr && length > 0)
							{
								const char first = value[0];
								const char end = value[length - 1];
								if ((first == '{' && end == '}') || (first == '[' && end == ']'))
								{
									if (!document.AddObject(key, value, length))
									{
										document.Add(key, value, length);
									}
								}
								else
								{
									document.Add(key, value, length);
								}
							}
							else
							{
								document.Add(key, value, length);
							}
							break;
						}
						case SQLITE_BLOB:
						{
							const int length = sqlite3_column_bytes(stmt, index);
							const char* value = (const char*)sqlite3_column_blob(stmt, index);
							if (value != nullptr && length > 0)
							{
								const char first = value[0];
								const char end = value[length - 1];
								if ((first == '{' && end == '}') || (first == '[' && end == ']'))
								{
									if (!document.AddObject(key, value, length))
									{
										document.Add(key, value, length);
									}
								}
								else
								{
									document.Add(key, value, length);
								}
							}
							else
							{
								document.Add(key, value, length);
							}
							break;
						}
						case SQLITE_NULL:
						{
							document.AddNull(key);
							break;
						}
						default:
						LOG_ERROR("[{}] unknown field type", sqlite3_column_type(stmt, index));
							break;
					}
				}
				std::unique_ptr<json::r::Document> document1
					= std::make_unique<json::r::Document>(document);
				{
					std::string json = document1->ToString();
					response->result.emplace_back(std::move(document1));
				}
				code = sqlite3_step(stmt);
			}
			response->ok = code == SQLITE_DONE;
		}
		sqlite3_reset(stmt);
		return response;
	}

	std::unique_ptr<sqlite::Response> SqliteComponent::Invoke(const std::string& name, lua_State* L)
	{
		auto iter = this->mStmtInfos.find(name);
		if(iter == this->mStmtInfos.end())
		{
			luaL_error(L, "not find stmt name : %s", name.c_str());
			return nullptr;
		}
		int top = lua_gettop(L);
		sqlite::ArgsBinder binder(iter->second);
		for(int index = 2; index <= top; index++)
		{
			switch(lua_type(L, index))
			{
				case LUA_TNUMBER:
				{
					if(lua_isinteger(L, index))
					{
						binder.Bind(lua_tointeger(L, index));
					}
					else
					{
						binder.Bind(lua_tonumber(L, index));
					}
					break;
				}
				case LUA_TSTRING:
				{
					size_t count = 0;
					const char * str = lua_tolstring(L, index, &count);
					binder.Bind(str, count);
					break;
				}
				case LUA_TTABLE:
				{
					wrap::string<true> json;
					if(lua::yyjson::read(L, index, json))
					{
						binder.Bind(json.c_str(), json.size());
					}
					break;
				}
			}
		}
		return this->Run(iter->second);
	}

	bool SqliteComponent::StartTransaction()
	{
		return this->Run("START TRANSACTION")->ok;
	}

	bool SqliteComponent::CommitTransaction()
	{
		return this->Run("COMMIT")->ok;
	}

	bool SqliteComponent::RollbackTransaction()
	{
		return this->Run("ROLLBACK")->ok;
	}
}
