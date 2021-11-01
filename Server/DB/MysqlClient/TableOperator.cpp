﻿#include "TableOperator.h"
#include <fstream>
#include <Protocol/db.pb.h>

namespace GameKeeper
{
    TableOperator::TableOperator(GKMysqlSocket *socket, const std::string db, rapidjson::Document &doc)
            : mDocument(doc)
    {
        this->mDataBase = db;
        this->mMysqlSocket = socket;
    }

	bool TableOperator::InitMysqlTable()
	{
		if (mysql_select_db(this->mMysqlSocket, this->mDataBase.c_str()) != 0)
		{
			const std::string sql = "CREATE DATABASE " + this->mDataBase;
			if (mysql_real_query(mMysqlSocket, sql.c_str(), sql.length()) != 0)
			{
				const char * err = mysql_error(mMysqlSocket);
				GKDebugError("create " << this->mDataBase << " db fail : " << err);
				return false;
			}
			if (mysql_select_db(this->mMysqlSocket, this->mDataBase.c_str()) == 0)
			{
				GKDebugInfo("create db " << this->mDataBase << " successful");
			}
		}

		for (auto iter = this->mDocument.MemberBegin(); iter != this->mDocument.MemberEnd(); iter++)
		{
			if (!iter->name.IsString() || !iter->value.IsObject())
			{
				GKDebugError("json config error");
				return false;
			}
			const std::string table = iter->name.GetString();
			const std::string name = iter->value.FindMember("protobuf")->value.GetString();
			auto iter1 = iter->value.FindMember("keys");
			if (iter1 == iter->value.MemberEnd() || !iter1->value.IsArray())
			{
				GKDebugError(table << " sql table config error");
				return false;
			}
			std::vector<std::string> keys;
			for (unsigned int index = 0; index < iter1->value.Size(); index++)
			{
				const std::string key = iter1->value[index].GetString();
				keys.push_back(key);
			}
			std::string sql = "desc " + table;
			if (mysql_real_query(this->mMysqlSocket, sql.c_str(), sql.length()) != 0)
			{
				if (!this->CreateMysqlTable(table, name, keys))
				{
					GKDebugError("create new table " << table << " fail " << mysql_error(this->mMysqlSocket));
					return false;
				}
				GKDebugInfo("create new table success " << table);
			}
			else
			{
				this->UpdateMysqlTable(table, name, keys);
			}
		}
		return true;
	}

    bool TableOperator::UpdateMysqlTable(const std::string table, const std::string name,
                                         const std::vector<std::string> &keys)
    {
        const DescriptorPool *pDescriptorPool = DescriptorPool::generated_pool();
        const Descriptor *pDescriptor = pDescriptorPool->FindMessageTypeByName(name);
        for (const std::string &key : keys)
        {
            if (pDescriptor->FindFieldByName(key) == nullptr)
            {
                GKDebugError("create " << name << " error 'key' not find");
                return false;
            }
        }
        MysqlQueryResult *queryResult = mysql_store_result(this->mMysqlSocket);
        if (queryResult == nullptr)
        {
            return false;
        }
        std::set<std::string> fieldSet;
        unsigned long rowCount = mysql_num_rows(queryResult);
        unsigned int fieldCount = mysql_field_count(mMysqlSocket);
        for (unsigned long count = 0; count < rowCount; count++)
        {
            MYSQL_ROW row = mysql_fetch_row(queryResult);
            unsigned long *lengths = mysql_fetch_lengths(queryResult);

            for (size_t index = 0; index < fieldCount; index++)
            {
                fieldSet.insert(std::string(row[index], (int) lengths[index]));
            }
        }

        for (int index = 1; index <= pDescriptor->field_count(); index++)
        {
            const FieldDescriptor *fileDesc = pDescriptor->FindFieldByNumber(index);
            if (fileDesc == nullptr)
            {
                continue;
            }
            auto iter = fieldSet.find(fileDesc->name());
            if (iter == fieldSet.end())
            {
                if (!this->AddNewField(table, fileDesc))
                {
                    GKDebugError("[mysql error ] " << mysql_error(this->mMysqlSocket));
                    GKDebugError("add field " << fileDesc->name() << " to " << table << " fail");
                    return false;
                }
                GKDebugLog("add field " << fileDesc->name() << " to " << table << " successful");
            }
        }
        return true;
    }

    bool TableOperator::AddNewField(const std::string table, const FieldDescriptor *fieldDesc)
    {
        std::stringstream sqlStream;
        sqlStream << "alter table " << table << " add " << fieldDesc->name();
        switch (fieldDesc->type())
        {
            case FieldDescriptor::TYPE_INT32:
                sqlStream << " int(20) DEFAULT 0";
                break;
            case FieldDescriptor::TYPE_UINT32:
                sqlStream << " INTEGER(32) DEFAULT 0";
                break;
            case FieldDescriptor::TYPE_INT64:
            case FieldDescriptor::TYPE_UINT64:
                sqlStream << " bigint(32) DEFAULT 0";
                break;
            case FieldDescriptor::TYPE_FLOAT:
                sqlStream << " float(20) DEFAULT 0";
                break;
            case FieldDescriptor::TYPE_DOUBLE:
                sqlStream << " double(32) DEFAULT 0";
                break;
            case FieldDescriptor::TYPE_STRING:
                sqlStream << " varchar(64) DEFAULT NULL";
                break;
            case FieldDescriptor::TYPE_BYTES:
                sqlStream << " BLOB(64) DEFAULT NULL";
                break;
            default:
                return false;
        }
        const std::string sql = sqlStream.str();
        if (mysql_real_query(this->mMysqlSocket, sql.c_str(), sql.length()) != 0)
        {
            GKDebugError(mysql_error(mMysqlSocket));
            return false;
        }
        GKDebugInfo("\n"
                               << sql);
        return true;
    }

    bool TableOperator::CreateMysqlTable(const std::string table, const std::string name,
                                         const std::vector<std::string> &keys)
    {
        db::UserAccountData account;
        const DescriptorPool *pDescriptorPool = DescriptorPool::generated_pool();
        const Descriptor *pDescriptor = pDescriptorPool->FindMessageTypeByName(name);
        for (const std::string &key : keys)
        {
            if (pDescriptor->FindFieldByName(key) == nullptr)
            {
                GKDebugError("create " << name << " error 'key' not find");
                return false;
            }
        }
        std::stringstream sqlCommand;
        auto IsHasField = [&keys](const std::string &name) -> bool {
            for (const std::string &key : keys)
            {
                if (key == name)
                {
                    return true;
                }
            }
            return false;
        };
        sqlCommand << "create table `" << table << "`(\n";
        for (int index = 1; index <= pDescriptor->field_count(); index++)
        {
            const FieldDescriptor *fileDesc = pDescriptor->FindFieldByNumber(index);
            if (fileDesc == nullptr)
            {
                continue;
            }
            sqlCommand << "`" << fileDesc->name() << "` ";
            if (fileDesc->type() == FieldDescriptor::TYPE_INT32)
            {
                if (IsHasField(fileDesc->name()))
                {
                    sqlCommand << "INT(20) NOT NULL,\n";
                    continue;
                }
                sqlCommand << "INT(20) DEFAULT 0,\n";
            }
            if (fileDesc->type() == FieldDescriptor::TYPE_UINT32)
            {
                if (IsHasField(fileDesc->name()))
                {
                    sqlCommand << "INTEGER(20) NOT NULL,\n";
                    continue;
                }
                sqlCommand << "INTEGER(20) DEFAULT 0,\n";
            } else if (fileDesc->type() == FieldDescriptor::TYPE_INT64 ||
                       fileDesc->type() == FieldDescriptor::TYPE_UINT64)
            {
                if (IsHasField(fileDesc->name()))
                {
                    sqlCommand << "BIGINT(32) NOT NULL,\n";
                    continue;
                }
                sqlCommand << "BIGINT(32) DEFAULT 0,\n";
            } else if (fileDesc->type() == FieldDescriptor::TYPE_FLOAT)
            {
                GKAssertRetFalse_F(IsHasField(fileDesc->name()));
                sqlCommand << "FLOAT(20) NOT NULL DEFAULT 0,\n";
            } else if (fileDesc->type() == FieldDescriptor::TYPE_DOUBLE)
            {
                GKAssertRetFalse_F(IsHasField(fileDesc->name()));
                sqlCommand << "DOUBLE(32) DEFAULT 0,\n";
            } else if (fileDesc->type() == FieldDescriptor::TYPE_STRING)
            {
                if (IsHasField(fileDesc->name()))
                {
                    sqlCommand << "VARCHAR(20) NOT NULL,\n";
                    continue;
                }
                sqlCommand << "VARCHAR(64) DEFAULT NULL,\n";
            } else if (fileDesc->type() == FieldDescriptor::TYPE_BYTES)
            {
                GKAssertRetFalse_F(IsHasField(fileDesc->name()));
                sqlCommand << "BLOB(64) DEFAULT NULL,\n";
            } else
            {
                return false;
            }
        }
        sqlCommand << "PRIMARY KEY (";
        for (size_t index = 0; index < keys.size(); index++)
        {
            const std::string &key = keys[index];
            if (index == keys.size() - 1)
            {
                sqlCommand << "`" << key << "`)\n";
                break;
            }
            sqlCommand << "`" << key << "`,";
        }
        sqlCommand << ")ENGINE=InnoDB DEFAULT CHARSET = utf8;";
        const std::string sql = sqlCommand.str();
        if (mysql_real_query(this->mMysqlSocket, sql.c_str(), sql.length()) != 0)
        {
            GKDebugError(mysql_error(mMysqlSocket));
            return false;
        }
        GKDebugInfo("\n" << sql);
        return true;
    }
}