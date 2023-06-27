//
// Created by mac on 2022/5/18.
//
#include"MongoProto.h"
#include"Mongo/Bson/bsonobj.h"
namespace Mongo
{
	MongoRequest::MongoRequest(int opcode)
	{
		this->header.requestID = 0;
		this->header.responseTo = 0;
		this->header.opCode = opcode;
		this->header.messageLength = 0;
	}

	int MongoRequest::Serialize(std::ostream& os)
	{
		int len = this->GetLength() + sizeof(MongoHead);
		this->Write(os, len);
		this->Write(os, this->header.requestID);
		this->Write(os, this->header.responseTo);
		this->Write(os, this->header.opCode);
		this->OnWriter(os);
		return 0;
	}
}

namespace Mongo
{
	CommandRequest::CommandRequest()
		: MongoRequest(OP_QUERY)
	{
		this->flag = 0;
		this->numberToSkip  = 0;
		this->numberToReturn = 1;
	}

	int CommandRequest::GetLength()
	{
		return sizeof(this->flag) + this->collectionName.size() 
			+ 1 + sizeof(int) * 2 + this->document.GetStreamLength();
	}
	void CommandRequest::OnWriter(std::ostream& os)
	{
		this->Write(os, this->flag);
		this->Write(os, this->collectionName);
		this->Write(os, this->numberToSkip);
		this->Write(os, this->numberToReturn);
		this->document.WriterToStream(os);
		//this->fields.WriterToStream(os);
	}
}

namespace Mongo
{
	int MongoInsert::GetLength()
	{
		return 0;
	}

	void MongoInsert::OnWriter(std::ostream& os)
	{

	}
}

namespace Mongo
{
    int CommandResponse::OnReceiveHead(std::istream & os)
    {
		union {
			MongoHead Head;
			char buffer[sizeof(MongoHead)];
		} head;
		os.readsome(head.buffer, sizeof(MongoHead));
		this->mHead = head.Head;
        return head.Head.messageLength - sizeof(MongoHead);
    }

    int CommandResponse::ReadInt(std::istream & is)
    {
        union {
            int v;
            char b[sizeof(int)];
        } u;
        is.readsome(u.b, sizeof(int));
        return u.v;
    }

    long long CommandResponse::ReadLong(std::istream &is)
    {
        union {
            long long v;
            char b[sizeof(long long)];
        } u;
        is.readsome(u.b, sizeof(long long));
        return u.v;
    }

	size_t CommandResponse::OnReceiveBody(std::istream & os)
	{
		this->mBuffer.clear();
        this->responseFlags = this->ReadInt(os);
        this->cursorID = this->ReadLong(os);
        this->startingFrom = this->ReadInt(os);
        this->numberReturned = this->ReadInt(os);

		size_t size = os.readsome(this->mReadBuffer, 128);
		while(size > 0)
		{
			this->mBuffer.append(this->mReadBuffer, size);
			size = os.readsome(this->mReadBuffer, 128);
		}
		const char * bson = this->mBuffer.c_str();
		this->mDocument = std::make_unique<Bson::Reader::Document>(bson);
		return 0;
	}
}