//
// Created by mac on 2022/5/19.
//

#include"MongoService.h"
#include"MongoRpcComponent.h"
namespace Sentry
{
    MongoService::MongoService()
        : mMongoComponent(nullptr)
    {

    }
	bool MongoService::OnStartService(ServiceMethodRegister& methodRegister)
    {
        this->mMongoComponent = this->GetComponent<MongoRpcComponent>();
        methodRegister.Bind("Query", &MongoService::Query);
        methodRegister.Bind("Insert", &MongoService::Insert);
        methodRegister.Bind("Delete", &MongoService::Delete);
        methodRegister.Bind("Update", &MongoService::Update);
        return this->mMongoComponent != nullptr;
    }

    XCode MongoService::Insert(const s2s::Mongo::Insert &request)
    {
        const std::string & tab = request.tab();
        const std::string & json = request.json();
        if(!this->mMongoComponent->InsertOnce(tab, json))
        {
#ifdef __DEBUG__
            LOG_ERROR("insert [" << tab << "] failure json =" << json);
#endif
            return XCode::Failure;
        }
        return XCode::Successful;
    }

    XCode MongoService::Delete(const s2s::Mongo::Delete &request)
    {
        int limit = request.limit();
        const std::string & tab = request.tab();
        const std::string & json = request.json();
        if(!this->mMongoComponent->Delete(tab, json, limit))
        {
#ifdef __DEBUG__
            LOG_ERROR("delete [" << tab << "] failure json =" << json);
#endif
            return XCode::Failure;
        }
        return XCode::Successful;
    }

    XCode MongoService::Update(const s2s::Mongo::Update &request)
    {
        const std::string & tab = request.tab();
        const std::string & tag = request.tag();
        const std::string & select = request.select();
        const std::string & update = request.update();
        if(!this->mMongoComponent->Update(tab, update, select, tag))
        {
#ifdef __DEBUG__
            LOG_ERROR("update [" << tab << "] failure select ="
                << select << " update = " << update);
#endif
            return XCode::Failure;
        }
        return XCode::Successful;
    }

    XCode MongoService::Query(const s2s::Mongo::Query::Request &request, s2s::Mongo::Query::Response &response)
    {
        int limit = request.limit();
        const std::string & tab = request.tab();
        const std::string & json = request.json();
        std::shared_ptr<MongoQueryResponse> queryResponse = this->mMongoComponent->Query(tab, json, limit);
        if(queryResponse == nullptr || queryResponse->GetDocumentSize() <= 0)
        {
            return XCode::Failure;
        }
        for(size_t index = 0; index < queryResponse->GetDocumentSize(); index++)
        {
            std::string * json = response.add_jsons();
            queryResponse->Get().WriterToJson(*json);
        }
        return XCode::Successful;
    }

}