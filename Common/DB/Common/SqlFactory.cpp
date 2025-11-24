//
// Created by 64658 on 2025/6/9.
//
#include <cassert>
#include "SqlFactory.h"
#include "fmt.h"
namespace sql
{

	void Factory::WriteValue(const json::r::Value& jsonValue)
	{
		switch (jsonValue.GetType())
		{
		case YYJSON_TYPE_ARR:
		case YYJSON_TYPE_OBJ:
			{
				size_t count = 0;
				std::unique_ptr<char> json;
				if(jsonValue.ToCString(json, count))
				{
					this->WriteSafeString(json.get(), count);
				}
				break;
			}
		case YYJSON_TYPE_STR:
			{
				size_t size = 0;
				const char * str = jsonValue.GetString(size);
				this->WriteSafeString(str, size);
				break;
			}
		case YYJSON_TYPE_NUM:
			{
				if(jsonValue.GetSubType() == YYJSON_SUBTYPE_REAL)
				{
					double value = 0;
					jsonValue.Get(value);
					this->buffer << value;
				}
				else
				{
					long long value = 0;
					jsonValue.Get(value);
					this->buffer << value;
				}
				break;
			}
		case YYJSON_TYPE_BOOL:
			{
				bool value = false;
				jsonValue.Get(value);
				this->buffer << (value ? "TRUE" : "FALSE");
				break;
			}
		case YYJSON_TYPE_NULL:
			{
				this->buffer << "NULL";
				break;
			}
		default:
			assert(false);
			break;
		}
	}


	void Factory::WriteValue(const std::vector<const char*>& keys, const json::r::Value& message)
	{
		json::r::Value jsonValue;
		for (size_t index = 0; index < keys.size(); index++)
		{
			message.Get(keys.at(index), jsonValue);
			this->WriteValue(jsonValue);
			if (index < keys.size() - 1)
			{
				this->buffer << ",";
			}
		}
	}

	Factory& Factory::Insert(const json::r::Value& document)
	{
		std::vector<const char*> keys = document.GetAllKey();
		this->buffer << "INSERT INTO " + this->tab << "(";
		this->Concat(keys, ',') << ")VALUES(";

		this->WriteValue(keys, document);
		this->buffer << ")";
		return *this;
	}

	Factory& Factory::Insert(const std::vector<std::string>& fields)
	{
		this->buffer << "INSERT INTO " + this->tab << "(";
		this->Concat(fields, ',') << ")VALUES(";
		for(size_t index = 0; index < fields.size(); index++)
		{
			this->buffer << '?';
			if(index < fields.size() - 1)
			{
				this->buffer << ',';
			}
		}
		this->buffer << ")";
		return *this;
	}

	std::stringstream & Factory::Concat(const std::vector<std::string>& arr, char cc)
	{
		for (size_t index = 0; index < arr.size(); index++)
		{
			const std::string & str = arr.at(index);
			this->buffer.write(str.c_str(), str.size());
			if (index < arr.size() - 1)
			{
				buffer << cc;
			}
		}
		return this->buffer;
	}

	std::stringstream& Factory::Concat(const std::vector<const char*>& arr, char cc)
	{
		for (size_t index = 0; index < arr.size(); index++)
		{
			this->buffer << arr.at(index);
			if (index < arr.size() - 1)
			{
				buffer << cc;
			}
		}
		return this->buffer;
	}

	Factory& Factory::Insert()
	{
		this->buffer << "INSERT INTO " + this->tab << "(";
		return *this;
	}

	Factory& Factory::Fields(const std::vector<std::string>& fields)
	{
		if(!fields.empty())
		{
			this->Concat(fields, ',') << ")VALUES";
		}
		return *this;
	}

	Factory& Factory::Values(const std::vector<std::string> & fields, const json::r::Value& document)
	{
		std::vector<const char*> keys;
		keys.reserve(fields.size());
		for(const std::string & key : fields)
		{
			keys.emplace_back(key.c_str());
		}
		this->buffer << "(";
		this->WriteValue(keys, document);
		this->buffer << ")";
		return *this;
	}

	Factory& Factory::Replace(const json::r::Value& document)
	{
		std::vector<const char*> keys = document.GetAllKey();
		this->buffer << "REPLACE INTO " + this->tab << "(";
		this->Concat(keys, ',') << ")VALUES(";

		this->WriteValue(keys, document);
		this->buffer << ")";
		return *this;
	}
}

namespace sql
{
	Factory& Factory::Filter(const std::string& filter)
	{
		if (!filter.empty())
		{
			this->buffer << " WHERE " << filter;
		}
		return *this;
	}

	Factory& Factory::Update(const json::r::Value& document)
	{
		size_t count = 0;
		this->buffer << "UPDATE " << this->tab << " SET ";
		if(document.GetType() == YYJSON_TYPE_STR)
		{
			const char * set = document.GetString(count);
			this->buffer.write(set, (std::streamsize )count);
			return *this;
		}
		if(document.GetType() == YYJSON_TYPE_OBJ)
		{
			json::r::Value jsonValue;
			std::vector<const char*> keys = document.GetAllKey();
			for (const char* key: keys)
			{
				count++;
				this->buffer << key << "=";
				if(document.Get(key, jsonValue))
				{
					this->WriteValue(jsonValue);
				}
				if(count < keys.size())
				{
					this->Next();
				}
			}
		}
		return *this;
	}

	Factory& Factory::Inc(const std::string& field, int value)
	{
		this->buffer << "UPDATE " << this->tab << " SET " << field << "=" << field;
		if(value > 0)
		{
			this->buffer << "+";
		}
		this->buffer << value;
		return *this;
	}

	Factory& Factory::Filter(const json::r::Value& filter)
	{
		if(filter.GetType() == YYJSON_TYPE_STR)
		{
			size_t size = 0;
			const char * str = filter.GetString(size);
			if(str != nullptr && size > 0)
			{
				this->buffer << " WHERE ";
				this->buffer.write(str, (std::streamsize )size);
			}
			return *this;
		}
		if(filter.IsArray())
		{
			std::string key;
			this->buffer << " WHERE ";
			size_t count = filter.MemberCount();
			for(size_t index = 0; index < count; index++)
			{
				json::r::Value jsonValue;
				json::r::Value firstValue;
				if (filter.Get(index, jsonValue) && jsonValue.GetFirst(key, firstValue))
				{
					this->buffer << key << '=';
					this->WriteValue(firstValue);
					if (index + 1 < count)
					{
						this->buffer << " OR ";
					}
				}
			}
		}
		else if(filter.IsObject())
		{
			std::vector<const char*> keys = filter.GetAllKey();
			if(keys.empty())
			{
				return *this;
			}
			this->buffer << " WHERE ";
			for (size_t index = 0; index < keys.size(); index++)
			{
				json::r::Value jsonValue;
				if (filter.Get(keys.at(index), jsonValue))
				{
					if(jsonValue.GetType() == YYJSON_TYPE_ARR)
					{
						json::r::Value jsonItem;
						this->buffer << keys.at(index) << " IN(";
						size_t arrayCount = jsonValue.MemberCount();
						for(size_t x = 0; x < arrayCount; x++)
						{
							if(jsonValue.Get(x, jsonItem))
							{
								this->WriteValue(jsonItem);
							}
							if(x < arrayCount - 1)
							{
								this->buffer << ",";
							}
						}
						this->buffer << ')';
					}
					else
					{
						this->buffer << keys.at(index) << '=';
						this->WriteValue(jsonValue);
					}
					if (index + 1 < keys.size())
					{
						this->buffer << " AND ";
					}
				}
			}
		}
		return *this;
	}

	Factory& Factory::Next(char c)
	{
		this->buffer << c;
		return *this;
	}

	Factory& Factory::Union(const std::string & tab)
	{
		this->tab = tab;
		this->buffer << " UNION ";
		return *this;
	}

	Factory& Factory::Select()
	{
		this->buffer << "SELECT * FROM " << this->tab;
		return *this;
	}

	Factory& Factory::From()
	{
		this->buffer << " FROM " << this->tab;
		return *this;
	}

	Factory& Factory::Func(const std::string& func, const std::string& field)
	{
		this->buffer << "SELECT " << func << "(" << field << ") AS count";
		return *this;
	}

	Factory& Factory::Append(const std::string& str)
	{
		this->buffer << str;
		return *this;
	}

	Factory& Factory::Func(const std::string& func, const std::string& field, const std::string& name)
	{
		this->buffer << "SELECT " << func << "(" << field << ") AS " << name;
		return *this;
	}

	Factory& Factory::Select(const json::r::Value& jsonValue)
	{
		size_t count = jsonValue.MemberCount();
		if(jsonValue.IsArray() && count > 0)
		{
			size_t size = 0;
			this->buffer << "SELECT ";
			for(size_t index = 0; index < count; index++)
			{
				const char * field = jsonValue.GetString(index, size);
				if(field != nullptr && size > 0)
				{
					this->buffer.write(field, (std::streamsize )size);
				}
				if(index < count - 1)
				{
					this->buffer << ',';
				}
			}
		}
		else
		{
			this->buffer << "SELECT *";
		}
		this->buffer << " FROM " << this->tab;
		return *this;
	}

	Factory& Factory::Select(const std::vector<std::string>& fields)
	{
		this->buffer << "SELECT ";
		if (fields.empty())
		{
			this->buffer << '*';
		}
		else
		{
			this->Concat(fields, ',');
		}
		this->buffer << " FROM " << this->tab;
		return *this;
	}

	Factory& Factory::Distinct(const json::r::Value& jsonValue)
	{
		size_t count = jsonValue.MemberCount();
		if(jsonValue.IsArray() && count > 0)
		{
			size_t size = 0;
			this->buffer << "SELECT DISTINCT ";
			for(size_t index = 0; index < count; index++)
			{
				const char * field = jsonValue.GetString(index, size);
				if(field != nullptr && size > 0)
				{
					this->buffer.write(field, (std::streamsize )size);
				}
				if(index < count - 1)
				{
					this->buffer << ',';
				}
			}
			this->buffer << " FROM " << this->tab;
		}
		return *this;
	}

	Factory& Factory::GetTable(const std::string& tab)
	{
		this->tab = tab;
		this->buffer.str("");
		return *this;
	}

	Factory& Factory::Execute(const std::string& stmt, json::r::Value& args)
	{
		this->buffer.str("");
		size_t count = args.MemberCount();
		this->buffer << "EXECUTE " << stmt << " USING ";
		for(size_t index = 0; index < count; index++)
		{
			this->buffer << fmt::format("@A{}", ++index);
			if(index < count - 1)
			{
				this->buffer << ',';
			}
		}
		return *this;
	}

	Factory& Factory::Set(json::r::Value& jsonArgs)
	{
		this->buffer.str("");
		this->buffer << "SET ";
		size_t length = jsonArgs.MemberCount();
		for(size_t index = 0; index < length; index++)
		{
			json::r::Value jsonValue;
			if(jsonArgs.Get(index, jsonValue))
			{
				this->buffer << fmt::format("@A{}=", ++index);
				this->WriteValue(jsonValue);
			}
			if(index < length -1)
			{
				this->buffer << ',';
			}
		}
		this->Next(';');
		return *this;
	}


	Factory& Factory::Execute(const std::string& stmt, std::vector<json::r::Value>& args)
	{
		this->buffer.str("");
		this->buffer << "EXECUTE " << stmt << "(";
		for(size_t index = 0; index < args.size(); index++)
		{
			json::r::Value & jsonValue = args.at(index);
			this->WriteValue(jsonValue);
			if(index < args.size() - 1)
			{
				this->buffer << ',';
			}
		}
		this->buffer << ')';
		return *this;
	}

	Factory& Factory::WriteSafeString(const char * str, size_t size)
	{
		this->buffer << "\'";
		const char* p = str;
		const char* end = p + size;
		const char* last = p;

		while (p < end) {
			const char* pos = static_cast<const char*>(memchr(p, '\'', end - p));
			if (!pos) break;

			this->buffer.write(p, pos - p);
			this->buffer << "''";
			p = pos + 1;
		}
		this->buffer.write(p, end - p);
		this->buffer << "\'";
		return *this;
	}

	Factory& Factory::NextTable(const std::string& tab)
	{
		this->tab = tab;
		return *this;
	}

	Factory& Factory::SetIndex(const std::string& field, bool unique)
	{
		if(!unique)
		{
			this->buffer << " CREATE INDEX " << "idx_" << field << " ON " << this->tab << "(" << field << ")";
			return *this;
		}
		this->buffer << " CREATE UNIQUE INDEX " << "idx_" << field << " ON " << this->tab << "(" << field << ")";
		return *this;
	}

	Factory& Factory::SetIndexNotExist(const std::string& field, bool unique)
	{
		if(!unique)
		{
			this->buffer << " CREATE INDEX IF NOT EXISTS " << "idx_" << field << " ON " << this->tab << "(" << field << ")";
			return *this;
		}
		this->buffer << " CREATE UNIQUE INDEX IF NOT EXISTS " << "idx_" << field << " ON " << this->tab << "(" << field << ")";
		return *this;
	}

	Factory& Factory::GroupBy(const std::string& field)
	{
		this->buffer << " GROUP BY " << field;
		return *this;
	}

	Factory& Factory::OrderBy(const json::r::Value& sorter)
	{
		if(sorter.IsObject())
		{
			std::string field;
			json::r::Value sortType;
			if(sorter.GetFirst(field, sortType))
			{
				size_t size = 0;
				const char * str = sortType.GetString(size);
				if(str != nullptr && size > 0)
				{
					this->OrderBy(field, std::string(str, size));
				}
			}
		}
		else if(sorter.GetType() == YYJSON_TYPE_STR)
		{
			std::string field;
			if(sorter.Get(field))
			{
				this->OrderBy(field, "DESC");
			}
		}
		return *this;
	}

	Factory& Factory::OrderBy(const std::string& field, const std::string& sorter)
	{
		this->buffer << " ORDER BY " << field << " " << sorter;
		return *this;
	}

	Factory& Factory::Like(const std::string& key, const std::string& value)
	{
		this->buffer << " WHERE " << key << " LIKE '" << value << "'";
		return *this;
	}

	Factory& Factory::Like(const std::string& field, const json::r::Value& filter, const std::string& value)
	{
		this->Filter(filter);
		this->buffer << " AND " << field << " LIKE '" << value << "'";
		return *this;
	}

	Factory& Factory::Limit(int limit)
	{
		if (limit > 0)
		{
			this->buffer << " LIMIT " << limit;
		}
		return *this;
	}

	Factory& Factory::Count()
	{
		this->buffer << "SELECT COUNT(*) AS count FROM " << this->tab;
		return *this;
	}

	Factory& Factory::Delete()
	{
		this->buffer << "DELETE FROM " << this->tab;
		return *this;
	}

	Factory& Factory::Offset(int offset, int limit)
	{
		this->buffer << " LIMIT " << offset << "," << limit;
		return *this;
	}

	Factory& Factory::Page(int page, int limit)
	{
		int offset = (page - 1) * limit;
		return this->Offset(offset, limit);
	}
}

namespace sql
{
	Factory& Factory::Alter()
	{
		this->buffer << "ALTER TABLE " << this->tab;
		return *this;
	}

	Factory& Factory::AlterColumn(const char * field)
	{
		this->buffer << "ALTER TABLE " << this->tab << " ADD COLUMN " << field;
		return *this;
	}

	Factory& Factory::AlterColumn(const char* field, const std::string& type)
	{
		this->buffer << "ALTER TABLE " << this->tab << " ADD COLUMN " << field << " " << type;
		return *this;
	}

	Factory& Factory::ModifyColumn(const char* field, const std::string& type)
	{
		this->buffer << "ALTER TABLE " << this->tab << " MODIFY COLUMN " << field << " " << type;
		return *this;
	}

	Factory& Factory::AlterPrimaryKey(const std::vector<std::string> & keys)
	{
		this->buffer << "ALTER TABLE " << this->tab << " ADD PRIMARY KEY(";
		this->Concat(keys, ',') << ")";
		return *this;
	}
}

namespace sql
{
	Factory& Factory::Begin()
	{
		this->buffer << "CREATE TABLE IF NOT EXISTS " << this->tab << "(";
		return *this;
	}

	Factory& Factory::End()
	{
		this->buffer << ")";
		return *this;
	}

	Factory& Factory::Comment(const std::string& comment)
	{
		this->buffer << " COMMENT '" << comment << "'";
		return *this;
	}

	Factory& Factory::Comment(const std::string& field, const std::string& comment)
	{
		this->buffer << "COMMENT ON COLUMN " << this->tab << "." << field << " IS '" << comment << "'";
		return *this;
	}

	Factory& Factory::Engine(const char* engine)
	{
		this->buffer << " ENGINE = " << engine;
		return *this;
	}

	Factory& Factory::Character(const char* character)
	{
		this->buffer << " CHARACTER SET = " << character;
		return *this;
	}

	Factory& Factory::Collate(const char* collate)
	{
		this->buffer << " COLLATE = " << collate;
		return *this;
	}

	Factory& Factory::AppendColumn(const char* field, const char* type)
	{
		this->buffer << field << " " << type;
		return *this;
	}

	Factory& Factory::Unique()
	{
		this->buffer << " UNIQUE";
		return *this;
	}

	Factory& Factory::Index(const std::string & field)
	{
		this->buffer << " INDEX idx_" << field << " (" << field << ")";
		return *this;
	}

	Factory& Factory::Index(const std::vector<std::string>& field)
	{
		for(size_t index = 0; index < field.size(); index++)
		{
			this->Index(field.at(index));
			if(index < field.size() - 1)
			{
				this->Next();
			}
		}
		return *this;
	}

	Factory& Factory::NotNull()
	{
		this->buffer << " NOT NULL";
		return *this;
	}

	Factory& Factory::AutoInc()
	{
		this->buffer << " AUTO_INCREMENT";
		return *this;
	}

	Factory& Factory::Default(int value)
	{
		this->buffer << " DEFAULT " << value;
		return *this;
	}

	Factory& Factory::Default()
	{
		this->buffer << " DEFAULT NULL";
		return *this;
	}

	Factory& Factory::Default(bool value)
	{
		this->buffer << " DEFAULT " << (value ? "TRUE" : "FALSE");
		return *this;
	}

	Factory& Factory::Default(const std::string& str)
	{
		this->buffer << " DEFAULT " << "'" << str << "'";
		return *this;
	}

	Factory& Factory::Default(const json::r::Value& defaultValue)
	{
		switch (defaultValue.GetType())
		{
			case YYJSON_TYPE_NUM:
			{
				int value = 0;
				defaultValue.Get(value);
				this->Default(value);
				break;
			}
			case YYJSON_TYPE_STR:
			{
				std::string value;
				defaultValue.Get(value);
				this->Default(value);
				break;
			}
			case YYJSON_TYPE_BOOL:
			{
				bool value = false;
				defaultValue.Get(value);
				this->Default(value);
				break;
			}
			case YYJSON_TYPE_NULL:
			{
				this->Default();
				break;
			}
		}
		return *this;
	}

	Factory& Factory::PrimaryKey(const std::vector<std::string>& keys)
	{
		if(!keys.empty())
		{
			this->buffer << "PRIMARY KEY (";
			this->Concat(keys, ',') << ')';
		}
		return *this;
	}
}