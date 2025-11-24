//
// Created by mac on 2022/5/18.
//

#ifndef APP_MONGOPROTO_H
#define APP_MONGOPROTO_H

#include"Proto/Message/IProto.h"
#include"Yyjson/Document/Document.h"
#include"Proto/Document/BsonDocument.h"

#define OP_REPLY 1
#define OP_MSG    1000
#define OP_UPDATE 2001
#define OP_INSERT 2002
#define OP_QUERY 2004
#define OP_GET_MORE    2005
#define OP_DELETE 2006
#define OP_KILL_CURSORS    2007
#ifdef __SHARE_PTR_COUNTER__
#include "Core/Memory/MemoryObject.h"
#endif
namespace mongo
{
    class Head final
    {
    public:
        int messageLength;
        int requestID;
        int responseTo;
        int opCode;
    };

	struct Info
	{
	public:
		std::string host;
		std::string memory;
		std::string version;
	};

	class Request final : public tcp::IProto
#ifdef __SHARE_PTR_COUNTER__
			, public memory::Object<Request>
#endif
    {
    public:
		Request();
	public:
		void Clear() final;
		std::string ToString() final;
		int OnSendMessage(std::ostream &os) final;
		int GetRpcId() const { return this->header.requestID; }
	public:
		void Insert(bson::w::Document & document);
	public:
		Request & GetCollection(const char * cmd, const std::string & tab);
	public:
		Request & Skip(int skip = 1);
		Request & Limit(int limit = 1);
		Request & Query(bson::w::Document & doc);
		bool Filter(bson::w::Document & doc);
		bool Filter(json::r::Value & filter);
	public:
        int flag;
		Head header;
		//std::string tab;
		std::string cmd;
		int numberToSkip;
		int numberToReturn;
		std::string dataBase;
		bson::w::Document document;
	};

	class Response final
#ifdef __SHARE_PTR_COUNTER__
			: public memory::Object<Response>
#endif
    {
    public:
		explicit Response(std::string cmd);
		Response(int id, std::string cmd);
    public:
		const Head & GetHead() const { return this->mHead;}
		inline int RpcId() const { return this->mHead.responseTo; }
		inline void SetRpcId(int id) { this->mHead.responseTo = id; }
		inline long long GetCursor() const { return this->cursorID; }
	public:
		std::string ToString();
		bool Encode(std::string * json);
		int OnRecvMessage(std::istream &os, size_t size);
		inline bool IsEmpty() const { return this->result.empty(); }
	private:
		bool DecodeQuery();
	private:
        Head mHead;
		int mDecodeState;
        int responseFlags;  // bit vector - see details below
        long long cursorID;       // cursor id if client needs to do get more's
        int startingFrom;   // where in the cursor this reply is starting
        int numberReturned; // number of documents in the reply
		const std::string cmd;
#ifdef __DEBUG__
		//std::string mJson;
#endif
	public:
		bson::r::Document document;
		std::list<std::string> result;
		std::unique_ptr<char[]> buffer;
    };
}


#endif //APP_MONGOPROTO_H
