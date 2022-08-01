//
// Created by mac on 2022/5/18.
//
#include"MongoProto.h"
#include"Bson/bsonobj.h"
namespace Mongo
{
	MongoRequest::MongoRequest(int opcode)
	{
		this->header.requestID = 0;
		this->header.responseTo = 0;
		this->header.opCode = opcode;
		this->header.messageLength = 0;
	}

	int MongoRequest::Serailize(std::ostream& os)
	{
		this->header.messageLength = this->GetLength() + sizeof(MongoHead);
		this->Write(os, this->header.messageLength);
		this->Write(os, this->header.requestID);
		this->Write(os, this->header.responseTo);
		this->Write(os, this->header.opCode);
		this->OnWriter(os);
		return 0;
	}
}

namespace Mongo
{
	int MongoLuaRequest::GetLength()
	{
		return this->mCommand.size();
	}
	void MongoLuaRequest::OnWriter(std::ostream& os)
	{
		os.write(this->mCommand.c_str(), this->mCommand.size());
	}

}

namespace Mongo
{
	MongoUpdateRequest::MongoUpdateRequest()
		: MongoRequest(OP_UPDATE)
	{
		this->flag = 0;
		this->zero = 0;
	}

	int MongoUpdateRequest::GetLength()
	{
		return 0;
	}

	void MongoUpdateRequest::OnWriter(std::ostream& os)
	{

	}
}

namespace Mongo
{
	MongoInsertRequest::MongoInsertRequest()
		: MongoRequest(OP_INSERT)
	{

	}

	void MongoInsertRequest::OnWriter(std::ostream& os)
	{

	}

	int MongoInsertRequest::GetLength()
	{
		return 0;
	}
}

namespace Mongo
{
	MongoQueryRequest::MongoQueryRequest()
		: MongoRequest(OP_QUERY)
	{
		this->flag = 0;
		this->numberToSkip  = 0;
		this->numberToReturn = 1;
		this->selector = nullptr;
	}

	int MongoQueryRequest::GetLength()
	{
		int size = this->selector != nullptr ? this->selector->GetStreamLength() : 0;
		return sizeof(this->flag) + this->collectionName.size() + 1 + sizeof(int) * 2 + this->document.GetStreamLength() + size;
	}
	void MongoQueryRequest::OnWriter(std::ostream& os)
	{
		this->Write(os, this->flag);
		this->Write(os, this->collectionName);
		this->Write(os, this->numberToSkip);
		this->Write(os, this->numberToReturn);
		this->document.WriterToStream(os);
		if(this->selector != nullptr)
		{
			this->selector->WriterToStream(os);
		}
		delete this->selector;
		this->selector = nullptr;
	}
}

namespace Mongo
{
    int MongoQueryResponse::OnReceiveHead(std::istream & os)
    {
        this->mHead.messageLength = this->ReadInt(os);
        this->mHead.requestID = this->ReadInt(os);
        this->mHead.responseTo = this->ReadInt(os);
        this->mHead.opCode = this->ReadInt(os);
        return this->mHead.messageLength - sizeof(MongoHead);
    }

    int MongoQueryResponse::ReadInt(std::istream & is)
    {
        union {
            int v;
            char b[sizeof(int)];
        } u;
        is.readsome(u.b, sizeof(int));
        return u.v;
    }

    long long MongoQueryResponse::ReadLong(std::istream &is)
    {
        union {
            long long v;
            char b[sizeof(long long)];
        } u;
        is.readsome(u.b, sizeof(long long));
        return u.v;
    }

	size_t MongoQueryResponse::OnReceiveBody(std::istream & os)
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
        for(int index = 0; index < this->mBuffer.size();)
        {
            const char * bson = this->mBuffer.c_str() + index;
            std::shared_ptr<Bson::Read::Object> obj(new Bson::Read::Object(bson));
            index += obj->Length();
            this->mDocuments.emplace_back(obj);
        }
		return this->mDocuments.size();
	}
}