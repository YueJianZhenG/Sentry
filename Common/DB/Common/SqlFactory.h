//
// Created by 64658 on 2025/6/9.
//

#ifndef APP_SQLFACTORY_H
#define APP_SQLFACTORY_H

#include <sstream>
#include "Yyjson/Document/Document.h"
namespace sql
{
	class Factory
	{
	public:
		Factory() = default;
	public:
		Factory & Insert();
		Factory & Insert(const json::r::Value & document);
		Factory & Insert(const std::vector<std::string> & fields);
	public:
		Factory & Replace(const json::r::Value & document);
		Factory & Fields(const std::vector<std::string> & fields);
		Factory & Values(const std::vector<std::string> & fields,const json::r::Value & document);
	public:
		Factory & Count();
		Factory & Delete();
		Factory & Limit(int limit);
		Factory & Page(int page, int limit);
		Factory & Offset(int offset, int limit);
		Factory & Update(const json::r::Value & update);
		Factory & Inc(const std::string & field, int value = 1);
		Factory & Like(const std::string & field, const std::string & value);
		Factory & Like(const std::string & field, const json::r::Value & filter, const std::string & value);
	public:
		Factory & Filter(const std::string & filter);
		Factory & Filter(const json::r::Value& filter);
	public:
		Factory & From();
		Factory & Append(const std::string & str);
		Factory & Func(const std::string & func, const std::string & field);
		Factory & Func(const std::string & func, const std::string & field, const std::string & name);
	public:
		Factory & Next(char c = ',');
		Factory & Union(const std::string & tab);
		Factory & Select();
		Factory & Select(const json::r::Value & jsonValue);
		Factory & Select(const std::vector<std::string> & fields);
	public:
		Factory & Distinct(const json::r::Value & jsonValue);
	public:
		Factory & Comment(const std::string & commend);
		Factory & Comment(const std::string & field, const std::string & commend);
	public:
		Factory & Set(json::r::Value & args); //mysql
		Factory & Execute(const std::string & stmt, json::r::Value & args); //mysql
		Factory & Execute(const std::string & stmt, std::vector<json::r::Value> & args); //pgsql
	public:
		Factory & End();
		Factory & Begin();
		Factory & Unique();
		Factory & NotNull();
		Factory & AutoInc(); //mysql
		Factory & Index(const std::string & field);
		Factory & Index(const std::vector<std::string> & field);
		Factory & Engine(const char * engine = "InnoDB");
		Factory & Character(const char * character = "utf8mb4");
		Factory & Collate(const char * collate = "utf8mb4_unicode_ci");

		Factory & PrimaryKey(const std::vector<std::string> & keys);
		Factory & AppendColumn(const char * field, const char * type);
		inline Factory & Int() { this->buffer << " INT"; return *this; }
		inline Factory & Bool() { this->buffer << " BOOL"; return *this; }
		inline Factory & Text() { this->buffer << " TEXT"; return *this; }
		inline Factory & Blob() { this->buffer << " BLOB"; return *this; }
		inline Factory & Json() { this->buffer << " JSON"; return *this; }
		inline Factory & Float() { this->buffer << " FLOAT"; return *this; }
		inline Factory & Double() { this->buffer << " DOUBLE"; return *this; }
		inline Factory & BigInt() { this->buffer << " BIGINT"; return *this; }
		inline Factory & LongText() { this->buffer << " LONGTEXT"; return *this; }
		inline Factory & LongBlob() { this->buffer << " LongBlob"; return *this; }
		inline Factory & AddColumn(const char * field) { this->buffer << field; return *this; }
		inline Factory & VarChar(int maxLen = 128) {this->buffer << " VARCHAR(" << maxLen << ")"; return *this; }
		inline Factory & Decimal(int max = 10, int dec = 2) { this->buffer << " DECIMAL(" << max << ',' << dec << ")"; return *this; }
	public:
		Factory & Alter();
		Factory & AlterColumn(const char * field);
		Factory & AlterColumn(const char * field, const std::string & type);
		Factory & ModifyColumn(const char * field, const std::string & type);
		Factory & AlterPrimaryKey(const std::vector<std::string> & keys);
	public:
		Factory & Default();
		Factory & Default(int value);
		Factory & Default(bool value);
		Factory & Default(const std::string& str);
		Factory & Default(const json::r::Value& value);
	public:
		Factory & GetTable(const std::string & tab);
		Factory & NextTable(const std::string & tab);
		Factory & GroupBy(const std::string & field);
		Factory & SetIndex(const std::string & field, bool unique);
		Factory & SetIndexNotExist(const std::string & field, bool unique);
	public:
		Factory & OrderBy(const json::r::Value & sorter);
		Factory & OrderBy(const std::string & field, const std::string & sorter);
	public:
		inline void Clear() { this->buffer.str(""); }
		Factory & WriteSafeString(const char * str, size_t size);
		std::string ToString() const { return this->buffer.str(); }
		std::stringstream & Concat(const std::vector<std::string> & arr, char cc);
		std::stringstream & Concat(const std::vector<const char *> & arr, char cc);
	private:
		void WriteValue(const json::r::Value & document);
		void WriteValue(const std::vector<const char*> & keys, const json::r::Value & document);
	private:
		std::string tab;
		std::stringstream buffer;
	};
}
#endif //APP_SQLFACTORY_H
