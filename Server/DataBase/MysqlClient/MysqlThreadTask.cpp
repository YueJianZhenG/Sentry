#include "MysqlThreadTask.h"
#include <Manager/MysqlManager.h>
#include <Coroutine/CoroutineManager.h>
#include <QueryResult/InvokeResultData.h>
namespace Sentry
{
	MysqlThreadTask::MysqlThreadTask(MysqlManager * mgr, const std::string &db, const std::string & sql)
		: mDataBaseName(db), mSqlCommand(sql)
	{
		this->mMysqlManager = mgr;
		SayNoAssertRet_F(this->mCorManager = mgr->GetManager<CoroutineManager>());
		this->mCoroutineId = this->mCorManager->GetCurrentCorId();
	}

	void MysqlThreadTask::OnTaskFinish()
	{
		if (this->mCorManager && this->mCoroutineId != 0)
		{
			this->mCorManager->Resume(this->mCoroutineId);
		}
	}

	void MysqlThreadTask::InvokeInThreadPool(long long threadId)
	{
		SayNoMysqlSocket *mysqlSocket = this->mMysqlManager->GetMysqlSocket(threadId);
		if (mysqlSocket == nullptr)
		{
			this->mErrorCode = MysqlSocketIsNull;
			this->mErrorString = "mysql socket is null";
			return;
		}

		if (mysql_select_db(mysqlSocket, this->mDataBaseName.c_str()) != 0)
		{
			this->mErrorCode = MysqlSelectDbFailure;
			this->mErrorString = "select " + this->mDataBaseName + " fail";
			return;
		}
		
		if (mysql_real_query(mysqlSocket, mSqlCommand.c_str(), mSqlCommand.size()) != 0)
		{
			this->mErrorCode = MysqlInvokeFailure;
			this->mErrorString = mysql_error(mysqlSocket);
			return;
		}
		this->mErrorCode = XCode::Successful;
		MysqlQueryResult *queryResult = mysql_store_result(mysqlSocket);
		if (queryResult != nullptr)
		{

			std::vector<MYSQL_FIELD *> fieldNameVector;
			unsigned long rowCount = mysql_num_rows(queryResult);
			unsigned int fieldCount = mysql_field_count(mysqlSocket);
			for (unsigned int index = 0; index < fieldCount; index++)
			{
				MYSQL_FIELD *field = mysql_fetch_field(queryResult);
				fieldNameVector.push_back(field);
			}
			if (rowCount == 1)
			{
				QuertJsonWritre jsonWrite;
				MYSQL_ROW row = mysql_fetch_row(queryResult);
				unsigned long *lengths = mysql_fetch_lengths(queryResult);
				for (size_t index = 0; index < fieldNameVector.size(); index++)
				{
					MYSQL_FIELD *field = fieldNameVector[index];
					this->WriteValue(jsonWrite, field, row[index], (int)lengths[index]);
				}
				std::string json;
				if (jsonWrite.Serialization(json))
				{
					this->mQueryDatas.push_back(json);
				}
			}
			else
			{

				for (unsigned long count = 0; count < rowCount; count++)
				{
					QuertJsonWritre jsonWrite;
					MYSQL_ROW row = mysql_fetch_row(queryResult);
					unsigned long *lengths = mysql_fetch_lengths(queryResult);
					for (size_t index = 0; index < fieldNameVector.size(); index++)
					{
						MYSQL_FIELD *field = fieldNameVector[index];
						this->WriteValue(jsonWrite, field, row[index], (int)lengths[index]);
					}
					std::string json;
					if (jsonWrite.Serialization(json))
					{
						this->mQueryDatas.push_back(json);
					}
				}
			}
			mysql_free_result(queryResult);
		}
	}

	void MysqlThreadTask::WriteValue(QuertJsonWritre &jsonWriter, MYSQL_FIELD *field, const char *data, long size)
	{
		switch (field->type)
		{
		case enum_field_types::MYSQL_TYPE_LONG:
		case enum_field_types::MYSQL_TYPE_LONGLONG:
			this->mValue2 = std::atoll(data);
			if (this->mValue2 != 0)
			{
				jsonWriter.Write(field->name, this->mValue2);
			}		
			break;
		case enum_field_types::MYSQL_TYPE_FLOAT:
		case enum_field_types::MYSQL_TYPE_DOUBLE:
			this->mValue1 = std::atof(data);
			if (this->mValue1 != 0)
			{
				jsonWriter.Write(field->name, this->mValue1);
			}		
		default:
			if (data != nullptr && size > 0)
			{
				jsonWriter.Write(field->name, data, size);
			}			
			break;
		}
	}
}