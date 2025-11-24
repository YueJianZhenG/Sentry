//
// Created by yy on 2025/7/6.
//

#include "MysqlProxyComponent.h"
#include "Mysql/Service/MysqlReadProxy.h"
#include "Mysql/Service/MysqlWriteProxy.h"
#include "Node/Component/NodeComponent.h"

#define GetTypeName(T) #T
#define ConcatName(T,str) GetTypeName(T) str

const std::string MysqlReadProxyName = GetTypeName(MysqlReadProxy);
const std::string MysqlWriteProxyName = GetTypeName(MysqlWriteProxy);

namespace acs
{
    MysqlProxyComponent::MysqlProxyComponent()
    {
        this->mNode = nullptr;
    }

    bool MysqlProxyComponent::LateAwake()
    {
        this->mNode = this->GetComponent<NodeComponent>();
        return true;
    }

    int MysqlProxyComponent::InsertOne(const char* tab, const pb::Message& message)
    {
        std::string document;
        if(!pb_json::MessageToJsonString(message, &document).ok())
        {
            return XCode::ProtoCastJsonFailure;
        }
        static std::string func = fmt::format("{}.InsertOne", MysqlReadProxyName);
        {
            json::w::Document request;
            request.Add("tab", tab);
            request.AddObject("document", document);
            return this->CallWriteProxy(func, request);
        }
    }

    int MysqlProxyComponent::InsertOne(const char* tab, const json::w::Document& document)
    {
        const static std::string func = fmt::format("{}.InsertOne", MysqlWriteProxyName);
        {
            json::w::Document request;
            request.Add("tab", tab);
            request.Add("document", document);
            return this->CallWriteProxy(func, request);
        }
    }

    int MysqlProxyComponent::ReplaceOne(const char* tab, const json::w::Document& document)
    {
		const static std::string func = fmt::format("{}.Replace", MysqlWriteProxyName);
		{
			json::w::Document request;
			request.Add("tab", tab);
			request.Add("document", document);
			return this->CallWriteProxy(func, request);
		}
    }

    int MysqlProxyComponent::DeleteOne(const char* tab, const json::w::Document& filter)
    {
		json::w::Document request;
		const static std::string func = fmt::format("{}.Delete", MysqlWriteProxyName);
        {
            request.Add("tab", tab);
            request.Add("limit", 1);
            request.Add("filter", filter);
        }
        return this->CallWriteProxy(func, request);
    }

    int MysqlProxyComponent::UpdateOne(const char* tab, const json::w::Document& filter, const json::w::Document& document)
    {
		json::w::Document request;
		const static std::string func = fmt::format("{}.Update", MysqlWriteProxyName);
        {
            request.Add("tab", tab);
            request.Add("limit", 1);
            request.Add("filter", filter);
            request.Add("document", document);
        }
        return this->CallWriteProxy(func, request);
    }

    int MysqlProxyComponent::CallWriteProxy(const std::string& func, const json::w::Document& request)
    {
        Node * mysqlProxy = this->mNode->Next(MysqlWriteProxyName);
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

    int MysqlProxyComponent::RunInRead(const std::string& sql, std::unique_ptr<json::r::Document>& response)
    {
		Node * mysqlProxy = this->mNode->Next(MysqlReadProxyName);
		const static std::string func = fmt::format("{}.Run", MysqlReadProxyName);
        if(mysqlProxy == nullptr)
        {
            return XCode::NotFoundActor;
        }
        return mysqlProxy->Call(func, sql, response);
    }


    int MysqlProxyComponent::RunInWrite(const std::string& sql, std::unique_ptr<json::r::Document>& response)
    {
        Node * mysqlProxy = this->mNode->Next(MysqlWriteProxyName);
		const static std::string func = fmt::format("{}.Run", MysqlWriteProxyName);
        if(mysqlProxy == nullptr)
        {
            return XCode::NotFoundActor;
        }
        return mysqlProxy->Call(func, sql, response);
    }

    long long MysqlProxyComponent::Inc(const char* tab, const char* field, const json::w::Document& filter, int value)
    {
        long long result = 0;
        do
        {
            Node * mysqlProxy = this->mNode->Next(MysqlWriteProxyName);
			const static std::string func = fmt::format("{}.Inc", MysqlWriteProxyName);
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

    int MysqlProxyComponent::FindOne(const char* tab, const json::w::Document& filter, std::unique_ptr<json::r::Document>& document)
    {
        Node * mysqlProxy = this->mNode->Next(MysqlWriteProxyName);
		const static std::string func = fmt::format("{}.FindOne", MysqlWriteProxyName);
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

    int MysqlProxyComponent::Find(const char* tab, const json::w::Document& filter, std::unique_ptr<json::r::Document>& document)
    {
        Node * mysqlProxy = this->mNode->Next(MysqlWriteProxyName);
		const static std::string func = fmt::format("{}.Find", MysqlWriteProxyName);
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

    int MysqlProxyComponent::Find(const char* tab, const std::list<std::string>& fields, const json::w::Document& filter, std::unique_ptr<json::r::Document>& document)
    {
        Node * mysqlProxy = this->mNode->Next(MysqlWriteProxyName);
		const static std::string func = fmt::format("{}.Find", MysqlWriteProxyName);
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

    int MysqlProxyComponent::FindOne(const char* tab, const std::list<std::string>& fields, const json::w::Document& filter, std::unique_ptr<json::r::Document>& document)
    {
        Node * mysqlProxy = this->mNode->Next(MysqlReadProxyName);
		const static std::string func = fmt::format("{}.FindOne", MysqlReadProxyName);
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

    long long MysqlProxyComponent::Count(const char* tab, const json::w::Document& filter)
    {
        long long result = 0;
        do
        {
            Node * mysqlProxy = this->mNode->Next(MysqlReadProxyName);
			const static std::string func = fmt::format("{}.Count", MysqlReadProxyName);
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