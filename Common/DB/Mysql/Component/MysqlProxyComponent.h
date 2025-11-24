//
// Created by yy on 2025/7/6.
//

#ifndef MYSQLPROXYCOMPONENT_H
#define MYSQLPROXYCOMPONENT_H
#include "Proto/Include/Message.h"
#include "Entity/Component/Component.h"
namespace acs
{
    class MysqlProxyComponent : public Component
    {
    public:
        MysqlProxyComponent();
    public:
        int InsertOne(const char * tab, const pb::Message & message);
        int InsertOne(const char * tab, const json::w::Document & document);
        int ReplaceOne(const char * tab, const json::w::Document & document);
    public:
        int DeleteOne(const char * tab, const json::w::Document & filter);
        int UpdateOne(const char * tab, const json::w::Document & filter, const json::w::Document & document);
    public:
        int RunInRead(const std::string & sql, std::unique_ptr<json::r::Document> & response);
        int RunInWrite(const std::string & sql, std::unique_ptr<json::r::Document> & response);
    public:
        long long Count(const char * tab, const json::w::Document & filter);
        long long Inc(const char * tab, const char * field, const json::w::Document & filter, int value = 1);
    public:
        int Find(const char * tab, const json::w::Document & filter, std::unique_ptr<json::r::Document> & document);
        int FindOne(const char * tab, const json::w::Document & filter, std::unique_ptr<json::r::Document> & document);
        int Find(const char * tab, const std::list<std::string> & fields, const json::w::Document & filter, std::unique_ptr<json::r::Document> & document);
        int FindOne(const char * tab, const std::list<std::string> & fields, const json::w::Document & filter, std::unique_ptr<json::r::Document> & document);
    private:
        int CallWriteProxy(const std::string & func, const json::w::Document & request);
    private:
        bool LateAwake() final;
    private:
        class NodeComponent * mNode;
    };
}





#endif //MYSQLPROXYCOMPONENT_H
