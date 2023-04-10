//
// Created by yjz on 2022/8/28.
//

#ifndef _MONGOAGENTCOMPONENT_H_
#define _MONGOAGENTCOMPONENT_H_
#include"Message/db.pb.h"
#include"Core/Component/Component.h"
using namespace google::protobuf;
namespace Tendo
{
	class MongoHelperComponent : public Component
	{
	 public:
		MongoHelperComponent() = default;
		~MongoHelperComponent() = default;
	 public:
		int Insert(const Message & message, int index);
		int Insert(const char * tab, const Message & message, int index = 0);
		int Insert(const char * tab, const std::string & json, int index = 0);
		int Remove(const char * tab, const std::string & select, int limit = 1, int index = 0);
		int Query(const char * tab, const std::string & select, std::shared_ptr<Message> response);
		int Update(const char * tab, const std::string & select, const std::string & data, int index);

    public:
		int Save(const Message & message);
		int Save(const char * tab, long long id, const std::string & data);
		int Save(const char * tab, const std::string & id, const std::string & data);
    private:
		bool LateAwake() final;
	 private:
		class RpcService* mMongoDB;
        db::mongo::update mUpdateRequest;
        db::mongo::insert mInsertRequest;
        db::mongo::remove mRemoveRequest;
		db::mongo::query::request mQueryRequest;
		class NodeMgrComponent * mLocationComponent;
	};
}

#endif //_MONGOAGENTCOMPONENT_H_