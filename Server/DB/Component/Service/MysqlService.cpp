﻿#include "MysqlService.h"
#include <Scene/MysqlComponent.h>
#include <Scene/TaskPoolComponent.h>
#include <MysqlClient/MysqlTaskProxy.h>
#include <Core/App.h>
#include <Pool/MessagePool.h>
namespace GameKeeper
{
    MysqlService::MysqlService()
    {
        this->mCorComponent = nullptr;
        this->mMysqlManager = nullptr;
    }

    bool MysqlService::Awake()
    {
		BIND_RPC_FUNCTION(MysqlService::Add);
		BIND_RPC_FUNCTION(MysqlService::Save);
		BIND_RPC_FUNCTION(MysqlService::Query);
		BIND_RPC_FUNCTION(MysqlService::Delete);
		BIND_RPC_FUNCTION(MysqlService::Invoke);
		this->mCorComponent = App::Get().GetCorComponent();
        LOG_CHECK_RET_FALSE(this->mMysqlManager = this->GetComponent<MysqlComponent>());
        LOG_CHECK_RET_FALSE(this->mTaskManager = this->GetComponent<TaskPoolComponent>());
		
        return true;
    }

    void MysqlService::Start()
    {
        db::UserAccountData userData;
        userData.set_account("646585122@qq.com");
        userData.set_userid(420625199511045331);
    }

    XCode MysqlService::Add(const s2s::MysqlOper_Request &request, s2s::MysqlOper_Response &response)
    {
        if(!request.has_data())
        {
            return XCode::CallArgsError;
        }
		Message * message = MessagePool::NewByData(request.data());
        if (message == nullptr)
        {
			return XCode::ParseMessageError;
        }
        std::string sql;
        if (!this->mMysqlManager->GetAddSqlCommand(*message, sql))
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__ && __MYSQL_DEBUG_LOG__
        std::cout <<"sql = " << sql << std::endl;
#endif
        std::shared_ptr<MysqlTaskProxy> mysqlTask =
                std::shared_ptr<MysqlTaskProxy>(new MysqlTaskProxy(this->mMysqlManager->GetDataBaseName(), sql));

        if (!this->mTaskManager->StartTask(mysqlTask.get()))
        {
            return XCode::MysqlStartTaskFail;
        }

        this->mCorComponent->WaitForYield();
        response.set_errorstr(mysqlTask->GetErrorStr());
        return mysqlTask->GetErrorCode();
    }

    XCode MysqlService::Save(const s2s::MysqlOper_Request &request, s2s::MysqlOper_Response &response)
    {
        if(!request.has_data())
        {
            return XCode::CallArgsError;
        }
		Message * message = MessagePool::New(request.data());
		if (message == nullptr)
		{
			return XCode::ParseMessageError;
		}
        
        std::string sql;
        if (!this->mMysqlManager->GetSaveSqlCommand(*message, sql))
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__ && __MYSQL_DEBUG_LOG__
        std::cout <<"sql = " << sql << std::endl;
#endif
        std::shared_ptr<MysqlTaskProxy> mysqlTask =
                std::shared_ptr<MysqlTaskProxy>(new MysqlTaskProxy(this->mMysqlManager->GetDataBaseName(), sql));

        if (!this->mTaskManager->StartTask(mysqlTask.get()))
        {
            return XCode::MysqlStartTaskFail;
        }

        this->mCorComponent->WaitForYield();
        response.set_errorstr(mysqlTask->GetErrorStr());
        return mysqlTask->GetErrorCode();
    }

    XCode MysqlService::Delete(const s2s::MysqlOper_Request &request, s2s::MysqlOper_Response &response)
    {
		Message * message = MessagePool::New(request.data());
		if (message == nullptr)
		{
			return XCode::ParseMessageError;
		}
        std::string sql;
        if (!this->mMysqlManager->GetDeleteSqlCommand(*message, sql))
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__ && __MYSQL_DEBUG_LOG__
        std::cout <<"sql = " << sql << std::endl;
#endif
        std::shared_ptr<MysqlTaskProxy> mysqlTask =
                std::shared_ptr<MysqlTaskProxy>(new MysqlTaskProxy(this->mMysqlManager->GetDataBaseName(), sql));

        if (!this->mTaskManager->StartTask(mysqlTask.get()))
        {
            return XCode::MysqlStartTaskFail;
        }
        this->mCorComponent->WaitForYield();
        response.set_errorstr(mysqlTask->GetErrorStr());
        return mysqlTask->GetErrorCode();
    }

    XCode MysqlService::Invoke(const s2s::MysqlAnyOper_Request &request, s2s::MysqlAnyOper_Response &response)
    {
        if(request.sql().empty())
        {
            return XCode::CallArgsError;
        }
        const SqlTableConfig * sqlTableConfig = this->mMysqlManager->GetTableConfig(request.tab());
        if(sqlTableConfig == nullptr)
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__ && __MYSQL_DEBUG_LOG__
        std::cout <<"sql = " << request.sql() << std::endl;
#endif
        std::shared_ptr<MysqlTaskProxy> mysqlTask = std::make_shared<MysqlTaskProxy>
                (this->mMysqlManager->GetDataBaseName(), request.sql());
        if (!this->mTaskManager->StartTask(mysqlTask.get()))
        {
            return XCode::MysqlStartTaskFail;
        }

        this->mCorComponent->WaitForYield();
        XCode code = mysqlTask->GetErrorCode();

        if (code == XCode::Successful)
        {
            std::string json;
            const std::string & name = sqlTableConfig->mProtobufName;
            while(mysqlTask->GetQueryData(json))
            {
                Message * message = MessagePool::NewByJson(name, json);
                if(message == nullptr)
                {
                    return XCode::JsonCastProtocbufFail;
                }
                response.add_querydatas()->PackFrom(*message);
            }
            return XCode::Successful;
        }
        response.set_errotstr(mysqlTask->GetErrorStr());
        return code;
    }

    XCode MysqlService::Query(const s2s::MysqlQuery_Request &request, s2s::MysqlQuery_Response &response)
    {
        if(!request.has_data())
        {
            return XCode::CallArgsError;
        }
		Message * message = MessagePool::NewByData(request.data());
        if (message == nullptr)
        {
            return XCode::ParseMessageError;
        }

        std::string sql;
        if (!this->mMysqlManager->GetQuerySqlCommand(*message, sql))
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__ && __MYSQL_DEBUG_LOG__
        std::cout <<"sql = " << sql << std::endl;
#endif
        std::shared_ptr<MysqlTaskProxy> mysqlTask =
                std::shared_ptr<MysqlTaskProxy>(new MysqlTaskProxy(this->mMysqlManager->GetDataBaseName(), sql));
        if (!this->mTaskManager->StartTask(mysqlTask.get()))
        {
            return XCode::MysqlStartTaskFail;
        }

        this->mCorComponent->WaitForYield();
        XCode code = mysqlTask->GetErrorCode();
        if (code == XCode::Successful)
        {
            std::string json;
            while(mysqlTask->GetQueryData(json))
            {
                Message * message = MessagePool::NewByJson(request.data(), json);
                if (message == nullptr)
                {
                    return XCode::JsonCastProtocbufFail;
                }
                response.add_querydatas()->PackFrom(*message);
            }
            return XCode::Successful;
        }
        response.set_errotstr(mysqlTask->GetErrorStr());
        return code;
    }
}// namespace GameKeeper