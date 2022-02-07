﻿#include"MysqlService.h"
#include"Component/MysqlComponent.h"
#include"Scene/ThreadPoolComponent.h"
#include"MysqlClient/MysqlTaskSource.h"
#include"Object/App.h"
#include"Pool/MessagePool.h"
#include"Other/ElapsedTimer.h"
namespace Sentry
{
    bool MysqlService::Awake()
    {
        this->mMysqlComponent = nullptr;
		BIND_RPC_FUNCTION(MysqlService::Add);
		BIND_RPC_FUNCTION(MysqlService::Save);
		BIND_RPC_FUNCTION(MysqlService::Query);
		BIND_RPC_FUNCTION(MysqlService::Delete);
		BIND_RPC_FUNCTION(MysqlService::Invoke);
        return true;
    }

    bool MysqlService::LateAwake()
    {
        this->mMysqlComponent = this->GetComponent<MysqlComponent>();
        if(this->mMysqlComponent == nullptr)
        {
            this->mMysqlComponent = this->mEntity->GetOrAddComponent<MysqlComponent>();
            LOG_CHECK_RET_FALSE(this->mMysqlComponent);
            return this->mMysqlComponent->LateAwake();
        }
        return true;
    }

    XCode MysqlService::Add(const s2s::MysqlOper_Request &request, s2s::MysqlResponse &response)
    {
        if(!request.has_data())
        {
            return XCode::CallArgsError;
        }
		Message * message = Helper::Proto::NewByData(request.data());
        if (message == nullptr)
        {
			return XCode::ParseMessageError;
        }
        std::string sql;
        if (!this->mMysqlComponent->GetAddSqlCommand(*message, sql))
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__
        ElapsedTimer timer;
#endif

        std::shared_ptr<MysqlTaskSource> taskSource(new MysqlTaskSource(this->mMysqlComponent));

        XCode code = taskSource->Await(sql);

#ifdef __DEBUG__
        std::cout << "add sql time = [" << timer.GetMs() << "ms]" << std::endl;
#endif
        if(code != XCode::Successful)
        {
            response.set_error(taskSource->GetErrorStr());
#ifdef __DEBUG__
            LOG_INFO(sql);
            LOG_ERROR(taskSource->GetErrorStr());
#endif
            return code;
        }
        return XCode::Successful;
    }

    XCode MysqlService::Save(const s2s::MysqlOper_Request &request, s2s::MysqlResponse &response)
    {
        if(!request.has_data())
        {
            return XCode::CallArgsError;
        }
		Message * message = Helper::Proto::NewByData(request.data());
		if (message == nullptr)
		{
			return XCode::ParseMessageError;
		}

        std::string sql;
        if (!this->mMysqlComponent->GetSaveSqlCommand(*message, sql))
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__
        ElapsedTimer timer;
#endif

        std::shared_ptr<MysqlTaskSource> taskSource(new MysqlTaskSource(this->mMysqlComponent));

        XCode code = taskSource->Await(sql);

#ifdef __DEBUG__
        std::cout << "save sql time = [" << timer.GetMs() << "ms]" << std::endl;
#endif
        if(code != XCode::Successful)
        {
#ifdef __DEBUG__
            LOG_INFO(sql);
            LOG_ERROR(taskSource->GetErrorStr());
#endif
            response.set_error(taskSource->GetErrorStr());
            return code;
        }
        return XCode::Successful;
    }

    XCode MysqlService::Delete(const s2s::MysqlOper_Request &request, s2s::MysqlResponse &response)
    {
		Message * message = Helper::Proto::NewByData(request.data());
		if (message == nullptr)
		{
			return XCode::ParseMessageError;
		}
        std::string sql;
        if (!this->mMysqlComponent->GetDeleteSqlCommand(*message, sql))
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__
        ElapsedTimer timer;
#endif
        std::shared_ptr<MysqlTaskSource> taskSource(new MysqlTaskSource(this->mMysqlComponent));

        XCode code = taskSource->Await(sql);

#ifdef __DEBUG__
        std::cout << "save sql time = [" << timer.GetMs() << "ms]" << std::endl;
#endif
        if(code != XCode::Successful)
        {
#ifdef __DEBUG__
            LOG_INFO(sql);
            LOG_ERROR(taskSource->GetErrorStr());
#endif
            response.set_error(taskSource->GetErrorStr());
            return code;
        }
        return XCode::Successful;
    }

    XCode MysqlService::Invoke(const s2s::MysqlAnyOper_Request &request, s2s::MysqlResponse &response)
    {
        if (request.sql().empty())
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__
      ElapsedTimer timer;
#endif
        std::string protoFullName;
        if(!this->mMysqlComponent->GetProtoByTable(request.tab(), protoFullName))
        {
            return XCode::CallArgsError;
        }
        std::shared_ptr<MysqlTaskSource> taskSource(new MysqlTaskSource(this->mMysqlComponent));

        XCode code = taskSource->Await(request.sql());

#ifdef __DEBUG__
        std::cout << "save sql time = [" << timer.GetMs() << "ms]" << std::endl;
#endif
        if (code != XCode::Successful)
        {
#ifdef __DEBUG__
            LOG_INFO(request.sql());
            LOG_ERROR(taskSource->GetErrorStr());
#endif
            response.set_error(taskSource->GetErrorStr());
            return code;
        }

        std::string json;
        while (taskSource->GetQueryData(json))
        {
            Message *message = Helper::Proto::NewByJson(protoFullName, json);
            if (message == nullptr)
            {
                return XCode::JsonCastProtoFailure;
            }
            response.add_datas()->PackFrom(*message);
        }
        return XCode::Successful;
    }

    XCode MysqlService::Query(const s2s::MysqlQuery_Request &request, s2s::MysqlResponse &response)
    {
        if (!request.has_data())
        {
            return XCode::CallArgsError;
        }
        Message *message = Helper::Proto::NewByData(request.data());
        if (message == nullptr)
        {
            return XCode::ParseMessageError;
        }

        std::string sql;
        if (!this->mMysqlComponent->GetQuerySqlCommand(*message, sql))
        {
            return XCode::CallArgsError;
        }
#ifdef __DEBUG__
      ElapsedTimer timer;
#endif
        std::shared_ptr<MysqlTaskSource> taskSource(new MysqlTaskSource(this->mMysqlComponent));

        XCode code = taskSource->Await(sql);
#ifdef __DEBUG__
        std::cout << "save sql time = [" << timer.GetMs() << "ms]" << std::endl;
#endif
        if (code != XCode::Successful)
        {
#ifdef __DEBUG__
            LOG_ERROR(taskSource->GetErrorStr());
#endif
            response.set_error(taskSource->GetErrorStr());
            return code;
        }

        std::string json;
        while (taskSource->GetQueryData(json))
        {
            Message *message = Helper::Proto::NewByJson(request.data(), json);
            if (message == nullptr)
            {
                return XCode::JsonCastProtoFailure;
            }
            response.add_datas()->PackFrom(*message);
        }
        return XCode::Successful;
    }
}// namespace Sentry