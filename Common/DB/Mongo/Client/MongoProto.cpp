//
// Created by mac on 2022/5/18.
//
#include "fmt.h"
#include"MongoProto.h"

#include <utility>
#include"Proto/Bson/bsonobj.h"

namespace mongo
{
	Request::Request()
	{
		this->flag = 0;
		this->numberToSkip  = 0;
		this->numberToReturn = 1;
		this->header.requestID = 0;
		this->header.responseTo = 0;
		this->header.opCode = OP_QUERY;
		this->header.messageLength = 0;
	}

	void Request::Clear()
	{
		this->flag = 0;
		this->numberToSkip  = 0;
		this->numberToReturn = 1;
		this->header.requestID = 0;
		this->header.responseTo = 0;
		this->header.opCode = OP_QUERY;
		this->header.messageLength = 0;
	}

	std::string Request::ToString()
	{
		return this->document.ToString();
	}

	int Request::OnSendMessage(std::ostream& os)
	{
		int length = 0;
		std::string cmd = fmt::format("{}.$cmd", this->dataBase);
		const char * bson = this->document.Serialize(length);
		int len = sizeof(this->flag) + cmd.size()
			   + 1 + sizeof(int) * 2 + length;

		len  += sizeof(Head);
		tcp::Data::Write(os, len);
		tcp::Data::Write(os, this->header.requestID);
		tcp::Data::Write(os, this->header.responseTo);
		tcp::Data::Write(os, this->header.opCode);

		tcp::Data::Write(os, this->flag);
		tcp::Data::Write(os, cmd);
		tcp::Data::Write(os, '\0');
		tcp::Data::Write(os, this->numberToSkip);
		tcp::Data::Write(os, this->numberToReturn);
		os.write(bson, length);

		return 0;
	}

	Request& Request::GetCollection(const char* cmd, const std::string& tab)
	{
		this->cmd = cmd;
		size_t  pos = tab.find('.');
		if(pos != std::string::npos)
		{
			this->dataBase = tab.substr(0, pos);
			std::string collection = tab.substr(pos + 1);
			this->document.Add(cmd, collection);
			return *this;
		}
		this->document.Add(cmd, tab);
		return *this;
	}
}

namespace mongo
{
	void Request::Insert(bson::w::Document& document)
	{
		if(document.IsArray())
		{
			this->document.Add("documents", document);
			return;
		}
		bson::w::Document documents(document);
		this->document.Add("documents", documents);
	}
}

namespace mongo
{

	Request& Request::Skip(int skip)
	{
		this->document.Add("skip", skip);
		return *this;
	}

	Request& Request::Limit(int limit)
	{
		this->document.Add("limit", limit);
		return *this;
	}

	Request& Request::Query(bson::w::Document& doc)
	{
		this->document.Add("query", doc);
		return *this;
	}

	bool Request::Filter(bson::w::Document& filter)
	{
//		bson::Writer::Document mode;
//		mode.Add("mode", "secondaryPreferred");
//		this->document.Add("$readPreference", mode);
		this->document.Add("filter", filter);
		return true;
	}

	bool Request::Filter(json::r::Value& filter)
	{
		bson::w::Document bsonFilter;
		do
		{
			size_t size = filter.MemberCount();
			if (size == 1)
			{
				std::string field;
				json::r::Value listValue;
				if (!filter.GetFirst(field, listValue) || !listValue.IsArray())
				{
					break;
				}
				bson::w::Document inArray(false);
				size_t count = listValue.MemberCount();
				for (size_t index = 0; index < count; index++)
				{
					json::r::Value listItem;
					listValue.Get(index, listItem);
					if (listItem.GetType() == YYJSON_TYPE_NUM)
					{
						long long number = 0;
						listItem.Get(number);
						if (number >= std::numeric_limits<int>::max())
						{
							inArray.Push(number);
						}
						else
						{
							inArray.Push((int)number);
						}
					}
					else if (listItem.GetType() == YYJSON_TYPE_STR)
					{
						size_t len = 0;
						const char* str = listItem.GetString(len);
						if (str != nullptr && len > 0)
						{
							inArray.Push(std::string(str, len));
						}
					}
				}
				bson::w::Document inObject;
				inObject.Add("$in", inArray);
				bsonFilter.Add(field.c_str(), inObject);
				{
					this->Filter(bsonFilter);
					//this->Limit(static_cast<int>(count));
				}
				return true;
			}
			if(!bsonFilter.FromByJson(filter))
			{
				return false;
			}
		}
		while(false);
		return this->Filter(bsonFilter);
	}
}


namespace mongo
{
	Response::Response(std::string cmd)
			: cmd(std::move(cmd))
	{
		this->mDecodeState = tcp::Decode::None;
	}

	Response::Response(int id, std::string cmd)
		: cmd(std::move(cmd))
	{
		this->mHead.responseTo = id;
	}

	std::string Response::ToString()
	{
		return this->document.ToString();
	}

	bool Response::Encode(std::string* json)
	{
		return this->document.WriterToJson(json);
	}

	bool Response::DecodeQuery()
	{
		bson::r::Document document1;
		if(!this->document.Get("cursor", document1))
		{
			std::string str = this->document.ToString();
			return false;
		}

		std::list<bson::r::Document> results;
		if(!document1.Get("firstBatch", results))
		{
			if(!document1.Get("nextBatch", results))
			{
				return false;
			}
		}
		document1.Get("id", this->cursorID);
		for(bson::r::Document & bsonDocument : results)
		{
			wrap::string<true> json;
			json::w::Document jsonWriter;
			bsonDocument.WriterToJson(jsonWriter);
			if(jsonWriter.Serialize(json))
			{
				this->result.emplace_back(json.c_str(), json.size());
			}
		}
		return true;
	}

	int Response::OnRecvMessage(std::istream& os, size_t size)
	{
		if (this->mDecodeState == tcp::Decode::None)
		{
			tcp::Data::Read(os, this->mHead);
			this->mDecodeState = tcp::Decode::MessageHead;
			return this->mHead.messageLength - (int)sizeof(this->mHead);
		}

		size_t offset = 0;
		{
			tcp::Data::Read(os, this->responseFlags);
			offset += sizeof(this->responseFlags);

			tcp::Data::Read(os, this->cursorID);
			offset += sizeof(this->cursorID);

			tcp::Data::Read(os, this->startingFrom);
			offset += sizeof(this->startingFrom);

			tcp::Data::Read(os, this->numberReturned);
			offset += sizeof(this->numberReturned);
		}
		size_t len = size - offset;
		this->buffer = std::make_unique<char[]>(len);
		{
			os.readsome(this->buffer.get(), len);
			this->document.Init(this->buffer.get());
//			this->mJson = this->mDocument.ToString();
//			this->mDocument.WriterToJson(&this->mJson);
			if(this->cmd == "find" || this->cmd == "aggregate" || this->cmd == "getMore")
			{
				this->DecodeQuery();
			}
		}
		return 0;
	}
}