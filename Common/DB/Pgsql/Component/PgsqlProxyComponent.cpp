//
// Created by yy on 2025/7/6.
//

#include "PgsqlProxyComponent.h"
#include "Pgsql/Service/PgsqlReadProxy.h"
#include "Pgsql/Service/PgsqlWriteProxy.h"
#include "Node/Component/NodeComponent.h"

#define GetTypeName(T) #T
#define ConcatName(T,str) GetTypeName(T) str

const std::string PgsqlReadProxyName = GetTypeName(PgsqlReadProxy);
const std::string PgsqlWriteProxyName = GetTypeName(PgsqlWriteProxy);

namespace acs
{
    PgsqlProxyComponent::PgsqlProxyComponent()
    {
        this->mNode = nullptr;
    }

    bool PgsqlProxyComponent::LateAwake()
    {
        this->mNode = this->GetComponent<NodeComponent>();
        return true;
    }

    int PgsqlProxyComponent::InsertOne(const char* tab, const pb::Message& message)
    {
        std::string document;
        if(!pb_json::MessageToJsonString(message, &document).ok())
        {
            return XCode::ProtoCastJsonFailure;
        }
        static std::string func = fmt::format("{}.InsertOne", PgsqlReadProxyName);
        {
            json::w::Document request;
            request.Add("tab", tab);
            request.AddObject("document", document);
            return this->CallWriteProxy(func, request);
        }
    }

    int PgsqlProxyComponent::InsertOne(const char* tab, const json::w::Document& document)
    {
        const static std::string func = fmt::format("{}.InsertOne", PgsqlWriteProxyName);
        {
            json::w::Document request;
            request.Add("tab", tab);
            request.Add("document", document);
            return this->CallWriteProxy(func, request);
        }
    }

    int PgsqlProxyComponent::ReplaceOne(const char* tab, const json::w::Document& document)
    {
		const static std::string func = fmt::format("{}.Replace", PgsqlWriteProxyName);
		{
			json::w::Document request;
			request.Add("tab", tab);
			request.Add("document", document);
			return this->CallWriteProxy(func, request);
		}
    }

    int PgsqlProxyComponent::DeleteOne(const char* tab, const json::w::Document& filter)
    {
		json::w::Document request;
		const static std::string func = fmt::format("{}.Delete", PgsqlWriteProxyName);
        {
            request.Add("tab", tab);
            request.Add("limit", 1);
            request.Add("filter", filter);
        }
        return this->CallWriteProxy(func, request);
    }

    int PgsqlProxyComponent::UpdateOne(const char* tab, const json::w::Document& filter, const json::w::Document& document)
    {
		json::w::Document request;
		const static std::string func = fmt::format("{}.Update", PgsqlWriteProxyName);
        {
            request.Add("tab", tab);
            request.Add("limit", 1);
            request.Add("filter", filter);
            request.Add("document", document);
        }
        return this->CallWriteProxy(func, request);
    }

    int PgsqlProxyComponent::CallWriteProxy(const std::string& func, const json::w::Document& request)
    {
        Node * mysqlProxy = this->mNode->Next(PgsqlWriteProxyName);
        if(mysqlProxy == nullptr)
        {
            return XCode::NotFoundActor;
        }
        std::unique_ptr<json::r::Document> response = std::make_unique<json::r::Document>();
        if( mysqlProxy->Call(func, request, response) != XCode::Ok)
        {
            return XCode::Failure;
        }
        int count = 0;
        return response->Get("count", count) && count >= 1 ? XCode::Ok : XCode::Failure;
    }

    int PgsqlProxyComponent::RunInRead(const std::string& sql, std::unique_ptr<json::r::Document>& response)
    {
		Node * mysqlProxy = this->mNode->Next(PgsqlReadProxyName);
		const static std::string func = fmt::format("{}.Run", PgsqlReadProxyName);
        if(mysqlProxy == nullptr)
        {
            return XCode::NotFoundActor;
        }
        return mysqlProxy->Call(func, sql, response);
    }


    int PgsqlProxyComponent::RunInWrite(const std::string& sql, std::unique_ptr<json::r::Document>& response)
    {
        Node * mysqlProxy = this->mNode->Next(PgsqlWriteProxyName);
		const static std::string func = fmt::format("{}.Run", PgsqlWriteProxyName);
        if(mysqlProxy == nullptr)
        {
            return XCode::NotFoundActor;
        }
        return mysqlProxy->Call(func, sql, response);
    }

    long long PgsqlProxyComponent::Inc(const char* tab, const char* field, const json::w::Document& filter, int value)
    {
        long long result = 0;
        do
        {
            Node * mysqlProxy = this->mNode->Next(PgsqlWriteProxyName);
			const static std::string func = fmt::format("{}.Inc", PgsqlWriteProxyName);
            if(mysqlProxy == nullptr)
            {
                result = -1;
                break;
            }
            json::w::Document request;
            {
                request.Add("tab", tab);
                request.Add("field", field);
                request.Add("value", value);
                request.Add("filter", filter);
            }
            std::unique_ptr<json::r::Document> response = std::make_unique<json::r::Document>();
            if(mysqlProxy->Call(func, request, response) != XCode::Ok)
            {
                result = 0;
            }
            response->Get("value", result);
        }
        while(false);
        return result;
    }

    int PgsqlProxyComponent::FindOne(const char* tab, const json::w::Document& filter, std::unique_ptr<json::r::Document>& document)
    {
        Node * mysqlProxy = this->mNode->Next(PgsqlWriteProxyName);
		const static std::string func = fmt::format("{}.FindOne", PgsqlWriteProxyName);
        if(mysqlProxy == nullptr)
        {
            return  XCode::NotFoundActor;
        }
        json::w::Document request;
        {
            request.Add("tab", tab);
            request.Add("filter", filter);
        }
        return  mysqlProxy->Call(func, request, document);
    }

    int PgsqlProxyComponent::Find(const char* tab, const json::w::Document& filter, std::unique_ptr<json::r::Document>& document)
    {
        Node * mysqlProxy = this->mNode->Next(PgsqlWriteProxyName);
		const static std::string func = fmt::format("{}.Find", PgsqlWriteProxyName);
        if(mysqlProxy == nullptr)
        {
            return  XCode::NotFoundActor;
        }
        json::w::Document request;
        {
            request.Add("tab", tab);
            request.Add("filter", filter);
        }
        return  mysqlProxy->Call(func, request, document);
    }

    int PgsqlProxyComponent::Find(const char* tab, const std::list<std::string>& fields, const json::w::Document& filter, std::unique_ptr<json::r::Document>& document)
    {
        Node * mysqlProxy = this->mNode->Next(PgsqlWriteProxyName);
		const static std::string func = fmt::format("{}.Find", PgsqlWriteProxyName);
        if(mysqlProxy == nullptr)
        {
            return  XCode::NotFoundActor;
        }
        json::w::Document request;
        {
            request.Add("tab", tab);
            request.Add("filter", filter);
            request.Add("fields", fields);
        }
        return  mysqlProxy->Call(func, request, document);
    }

    int PgsqlProxyComponent::FindOne(const char* tab, const std::list<std::string>& fields, const json::w::Document& filter, std::unique_ptr<json::r::Document>& document)
    {
        Node * mysqlProxy = this->mNode->Next(PgsqlReadProxyName);
		const static std::string func = fmt::format("{}.FindOne", PgsqlReadProxyName);
        if(mysqlProxy == nullptr)
        {
            return  XCode::NotFoundActor;
        }
        json::w::Document request;
        {
            request.Add("tab", tab);
            request.Add("filter", filter);
            request.Add("fields", fields);
        }
        return  mysqlProxy->Call(func, request, document);
    }

    long long PgsqlProxyComponent::Count(const char* tab, const json::w::Document& filter)
    {
        long long result = 0;
        do
        {
            Node * mysqlProxy = this->mNode->Next(PgsqlReadProxyName);
			const static std::string func = fmt::format("{}.Count", PgsqlReadProxyName);
            if(mysqlProxy == nullptr)
            {
                result = -1;
                break;
            }
            json::w::Document request;
            {
                request.Add("tab", tab);
                request.Add("filter", filter);
            }
            std::unique_ptr<json::r::Document> response = std::make_unique<json::r::Document>();
            if(mysqlProxy->Call(func, request, response) != XCode::Ok)
            {
                result = 0;
                break;
            }
            response->Get("count", result);
        }
        while(false);
        return result;
    }
}