﻿#include"MysqlTableTaskSource.h"
#include"fstream"
#include"App/App.h"
#include"Protocol/db.pb.h"
using namespace google::protobuf;
namespace Sentry
{
	MysqlTableTaskSource::MysqlTableTaskSource(const std::string& name)
		: mName(name)
	{

	}
	void MysqlTableTaskSource::Run(MysqlSocket* mysql)
	{
		const DescriptorPool * descriptorPool = DescriptorPool::generated_pool();
		const FileDescriptor * desc = descriptorPool->FindFileByName(this->mName);
		for (int x = 0; x < desc->message_type_count(); x++)
		{
			const Descriptor * messageDesc = desc->message_type(x);
			if(!this->InitDb(mysql, messageDesc->name()))
			{
				this->mTaskSource.SetResult(XCode::Failure);
				return;
			}
			for (int y = 0; y < messageDesc->nested_type_count(); y++)
			{
				const Descriptor * desc = messageDesc->nested_type(y);
				if(desc == nullptr || !this->InitTable(mysql, desc))
				{
					this->mTaskSource.SetResult(XCode::Failure);
					return;
				}
			}
		}
		this->mTaskSource.SetResult(XCode::Successful);
	}

	XCode MysqlTableTaskSource::Await()
	{
		XCode code = this->mTaskSource.Await();
		if(code != XCode::Successful)
		{
			LOG_ERROR(this->mError);
			return code;
		}
		return XCode::Successful;
	}

    bool MysqlTableTaskSource::InitDb(MysqlSocket * mysql, const std::string &db)
    {
        if (mysql_select_db(mysql, db.c_str()) != 0)
        {
            const std::string sql = "CREATE DATABASE " + db;
            if (mysql_real_query(mysql, sql.c_str(), sql.length()) != 0)
            {
				this->mError = mysql_error(mysql);
                return false;
            }
            return mysql_select_db(mysql, db.c_str()) == 0;
        }
        return true;
    }

    bool MysqlTableTaskSource::InitTable(MysqlSocket * mysql, const Descriptor *typeDescriptor)
    {
        std::string sql = "desc " + typeDescriptor->name();
        if (mysql_real_query(mysql, sql.c_str(), sql.length()) != 0)
        {
            if (!this->CreateMysqlTable(mysql, typeDescriptor))
            {
                return false;
            }
            std::cout <<"create new table {0} successful " << typeDescriptor->name() << std::endl;
        }
        else if (!this->UpdateMysqlTable(mysql, typeDescriptor))
        {
            return false;
        }
        return true;
    }

    bool MysqlTableTaskSource::UpdateMysqlTable(MysqlSocket * mysql, const Descriptor *descriptor)
    {
        MysqlQueryResult *queryResult = mysql_store_result(mysql);
        if (queryResult == nullptr)
        {
            return false;
        }
        std::set<std::string> fieldSet;
        unsigned long rowCount = mysql_num_rows(queryResult);
        unsigned int fieldCount = mysql_field_count(mysql);
        for (unsigned long count = 0; count < rowCount; count++)
        {
            MYSQL_ROW row = mysql_fetch_row(queryResult);
            unsigned long *lengths = mysql_fetch_lengths(queryResult);

            for (size_t index = 0; index < fieldCount; index++)
            {
                fieldSet.insert(std::string(row[index], (int) lengths[index]));
            }
        }

        for (int index = 1; index <= descriptor->field_count(); index++)
        {
            const FieldDescriptor *fileDesc = descriptor->FindFieldByNumber(index);
            if (fileDesc == nullptr)
            {
                continue;
            }
            auto iter = fieldSet.find(fileDesc->name());
            if (iter == fieldSet.end())
            {
                if (!this->AddNewField(mysql, descriptor->name(), fileDesc))
                {
					this->mError = mysql_error(mysql);
                    return false;
                }
                std::cout << "add field " << fileDesc->name() << " to " << descriptor->name() << " successful" << std::endl;
            }
        }
        return true;
    }

    bool MysqlTableTaskSource::AddNewField(MysqlSocket * mysql, const std::string &table, const FieldDescriptor *fieldDesc)
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
        if (mysql_real_query(mysql, sql.c_str(), sql.length()) != 0)
        {
			this->mError = mysql_error(mysql);
			return false;
        }
        LOG_INFO(sql);
        return true;
    }

    bool MysqlTableTaskSource::CreateMysqlTable(MysqlSocket * mysql, const Descriptor *descriptor)
    {
        std::stringstream sqlCommand;
		const FieldDescriptor *fileDesc = descriptor->FindFieldByNumber(1);
		switch(fileDesc->type())
		{
		case FieldDescriptor::TYPE_INT32:
		case FieldDescriptor::TYPE_INT64:
		case FieldDescriptor::TYPE_UINT32:
		case FieldDescriptor::TYPE_UINT64:
		case FieldDescriptor::TYPE_STRING:
			break;
		default:
			this->mError = fmt::format("[{0}.{1}] type error", descriptor->name(),  fileDesc->name());
			return false;
		}
        sqlCommand << "create table `" << descriptor->name() << "`(\n";
        for (int index = 1; index <= descriptor->field_count(); index++)
        {
            fileDesc = descriptor->FindFieldByNumber(index);
            if (fileDesc == nullptr)
            {
                continue;
            }
            sqlCommand << "`" << fileDesc->name() << "` ";
            switch (fileDesc->type())
            {
                case FieldDescriptor::TYPE_INT32:
                    sqlCommand << (index == 1 ? "INT(20) NOT NULL,\n" : "INT(20) DEFAULT 0,\n");
                    break;
                case FieldDescriptor::TYPE_UINT32:
                    sqlCommand << (index == 1 ? "INTEGER(20) NOT NULL,\n" : "INTEGER(20) DEFAULT 0,\n");
                    break;
                case FieldDescriptor::TYPE_INT64:
                case FieldDescriptor::TYPE_UINT64:
                    sqlCommand << (index == 1 ? "INTEGER(20) NOT NULL,\n" : "BIGINT(32) DEFAULT 0,\n");
                    break;
                case FieldDescriptor::CPPTYPE_FLOAT:
                LOG_CHECK_RET_FALSE(index > 1);
                    sqlCommand << "FLOAT(20) NOT NULL DEFAULT 0,\n";
                    break;
                case FieldDescriptor::TYPE_DOUBLE:
                LOG_CHECK_RET_FALSE(index > 1);
                    sqlCommand << "DOUBLE(20) NOT NULL DEFAULT 0,\n";
                    break;
                case FieldDescriptor::TYPE_STRING:
                    sqlCommand << (index == 1 ? "VARCHAR(64) NOT NULL,\n" : "VARCHAR(64) DEFAULT NULL,\n");
                    break;
                case FieldDescriptor::TYPE_BYTES:
                    sqlCommand << "BLOB(64) DEFAULT NULL,\n";
                    break;
                default:
					this->mError = fmt::format("[{0}.{1}] type error", descriptor->name(),  fileDesc->name());
				return false;
            }
        }
        fileDesc = descriptor->FindFieldByNumber(1);
		sqlCommand << "UNIQUE KEY (`" << fileDesc->name() << "`),\n";
		sqlCommand << "PRIMARY KEY (`" << fileDesc->name() << "`)";
        sqlCommand << ")ENGINE=InnoDB DEFAULT CHARSET = utf8;";
        const std::string sql = sqlCommand.str();
        if (mysql_real_query(mysql, sql.c_str(), sql.length()) != 0)
        {
			this->mError = mysql_error(mysql);
            return false;
        }
        return true;
    }
}