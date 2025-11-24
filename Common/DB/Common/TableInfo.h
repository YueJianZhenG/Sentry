//
// Created by 64658 on 2025/6/12.
//

#ifndef APP_TABLEINFO_H
#define APP_TABLEINFO_H

#include "Yyjson/Object/JsonObject.h"
namespace sql
{
	const std::string LOG_MYSQL = "mysql";
	const std::string LOG_PGSQL = "pgsql";
	struct Field : public json::Object<Field>
	{
	public:
		std::string name;
		std::string type;
		std::string comment;
		bool index = false;
		bool unique = false;
		bool notnull = false;
		bool auto_inc = false;
		bool primary_key = false;
		json::r::Value default_val;
	};

	struct Table : public json::Object<Table>
	{
	public:
		int count = 0;
		std::string name;
		std::string schema;
		std::unordered_map<std::string, Field> fields;
	protected:
		bool OnDecodeOk() final
		{
			auto iter = this->fields.begin();
			for(; iter != this->fields.end(); iter++)
			{
				iter->second.name = iter->first;
			}
			return true;
		}
	};

	inline void RegisterObject()
	{
		Field::RegisterField("name", &Field::name);
		Field::RegisterField("type", &Field::type);
		Field::RegisterField("index", &Field::index);
		Field::RegisterField("unique", &Field::unique);
		Field::RegisterField("notnull", &Field::notnull);
		Field::RegisterField("comment", &Field::comment);
		Field::RegisterField("auto_inc", &Field::auto_inc);
		Field::RegisterField("default", &Field::default_val);
		Field::RegisterField("primary_key", &Field::primary_key);

		Table::RegisterField("count", &Table::count);
		Table::RegisterField("schema", &Table::schema);
		Table::RegisterField("name", &Table::name, true);
		Table::RegisterField("fields", &Table::fields, true);
	}
}

#endif //APP_TABLEINFO_H
