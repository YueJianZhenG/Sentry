//
// Created by zmhy0073 on 2022/8/25.
//
#ifdef __ENABLE_MYSQL__
#ifndef APP_MYSQLMESSAGE_H
#define APP_MYSQLMESSAGE_H
#ifdef _WIN32
#include<WinSock2.h>
#include<windows.h>
#endif
#include"mysql.h"
#include<memory>
#include<string>
#include<sstream>
#include"google/protobuf/message.h"
using namespace google::protobuf;
namespace Mysql
{
	class Response
	{
    public:
        Response(MYSQL_RES * result);
        Response(const std::string & error);
        ~Response();
    public:
        bool HasError() const { return !this->mError.empty();}
        const std::string & GetError() const { return this->mError;}
    private:
        std::string mError;
        MYSQL_RES * mResult;
	};
    class ICommand
	{
	 public:
        ICommand(int id) : mRpcId(id) { }
        int GetRpcId() const{ return this->mRpcId;}
        virtual MYSQL_RES * Invoke(MYSQL *, std::string & error) = 0;
    private:
        int mRpcId;
	};

    class PingCommand : public ICommand
    {
    public:
        using ICommand::ICommand;
        MYSQL_RES * Invoke(MYSQL *, std::string &error) final;
    };

	class SqlCommand : public ICommand
	{
	 public:
        using ICommand::ICommand;
        MYSQL_RES * Invoke(MYSQL *, std::string & error) final;
	};

    class QueryCommand : public ICommand
    {
    public:
        using ICommand::ICommand;
        MYSQL_RES * Invoke(MYSQL *, std::string &error) final;
    };
}

namespace Mysql
{
    class CreateTabCommand : public ICommand
    {
    public:
        CreateTabCommand(std::shared_ptr<google::protobuf::Message> message, int id);

    public:
        MYSQL_RES * Invoke(MYSQL *, std::string &error) final;

    private:
        bool ForeachMessage(const FieldDescriptor * field);
    private:
        std::stringstream mBuffer;
        std::shared_ptr<google::protobuf::Message> mMessage;
    };
}


#endif //APP_MYSQLMESSAGE_H
#endif
