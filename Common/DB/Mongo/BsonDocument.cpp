//
// Created by mac on 2022/6/27.
//

#include "BsonDocument.h"

namespace Bson
{
	namespace Writer
	{
		void Array::Add(int value)
		{
			this->append(std::to_string(this->mIndex++), value);
		}

		void Array::Add(bool value)
		{
			this->append(std::to_string(this->mIndex++), value);
		}

		void Array::Add(long long value)
		{
			this->append(std::to_string(this->mIndex++), value);
		}

		void Array::Add(const std::string &value)
		{
			this->append(std::to_string(this->mIndex++), value);
		}

		void Array::Add(const char *str, size_t size)
		{
			this->append(std::to_string(this->mIndex++), str, size + 1);
		}

		void Array::Add(unsigned int value)
		{
			this->append(std::to_string(this->mIndex++), value);
		}

		void Array::Add(double value)
		{
			this->append(std::to_string(this->mIndex++), value);
		}

		void Array::Add(Array &document)
		{
			_bson::bsonobjbuilder &build = (_bson::bsonobjbuilder &)document;
			_b.appendNum((char)_bson::Array);
			_b.appendStr(std::to_string(this->mIndex++));

			const char * str = build._done();

			_b.appendBuf(str, build.len());
		}

		void Array::Add(Document &document)
		{
			_bson::bsonobjbuilder &build = (_bson::bsonobjbuilder &)document;

			_b.appendNum((char)_bson::Object);
			_b.appendStr(std::to_string(this->mIndex++));

			const char * str = build._done();
			_b.appendBuf(str, build.len());
		}
	}
}

namespace Bson
{
	namespace Reader
	{
		Document::Document(const char *bson)
			: _bson::bsonobj(bson)
		{
		}

		bool Document::IsOk() const
		{
			double isOk = 0;
			return this->Get("ok", isOk) && isOk != 0;
		}

		bool Document::Get(const char *key, double &value) const
		{
			_bson::bsonelement bsonelement = this->getField(key);
			switch (bsonelement.type())
			{
			case _bson::BSONType::NumberDouble:
				value = bsonelement.Double();
				return true;
			}
			return false;
		}
	}
	namespace Writer
	{

		void Document::WriterToJson(std::string &json)
		{
			int length = 0;
			Bson::Reader::Document obj(this->Serialize(length));
			obj.WriterToJson(json);
		}

		bool Document::FromByJson(const std::string &json)
		{
			rapidjson::Document document;
			if (document.Parse(json.c_str(), json.size()).HasParseError())
			{
				return false;
			}
			auto iter = document.MemberBegin();
			for (; iter != document.MemberEnd(); iter++)
			{
				const char *key = iter->name.GetString();
				if (!this->WriterToBson(key, *this, iter->value))
				{
					return false;
				}
			}
			return true;
		}

        bool Document::FromByJson(const std::string &json, std::string &id)
        {
            rapidjson::Document document;
            if (document.Parse(json.c_str(), json.size()).HasParseError())
            {
                return false;
            }
            auto findIter = document.FindMember("_id");
            if(findIter != document.MemberEnd())
            {
                rapidjson::Value & value = findIter->value;
                if(value.IsInt())
                {
                    id = std::to_string(value.GetInt());
                }
                else if(value.IsInt64())
                {
                    id = std::to_string(value.GetInt64());
                }
                else if(value.IsUint())
                {
                    id = std::to_string(value.GetUint());
                }
                else if(value.IsUint64())
                {
                    id = std::to_string(value.GetUint64());
                }
                else if(value.IsString())
                {
                    id = value.GetString();
                }
            }

            auto iter = document.MemberBegin();
            for (; iter != document.MemberEnd(); iter++)
            {
                const char *key = iter->name.GetString();
                const rapidjson::Value & value = iter->value;
                if (!this->WriterToBson(key, *this, iter->value))
                {
                    return false;
                }
            }
            return true;
        }

		bool Document::WriterToBson(Array& document, const rapidjson::Value& jsonValue)
		{
			if (jsonValue.IsString())
			{
				const char *str = jsonValue.GetString();
				const size_t size = jsonValue.GetStringLength();
				document.Add(str, size);
				return true;
			}
			else if (jsonValue.IsBool())
			{
				document.Add(jsonValue.GetBool());
				return true;
			}
			else if (jsonValue.IsInt())
			{
				document.Add(jsonValue.GetInt());
				return true;
			}
			else if (jsonValue.IsInt64())
			{
				document.Add((long long)jsonValue.GetInt64());
				return true;
			}
			else if (jsonValue.IsUint())
			{
				document.Add(jsonValue.GetUint());
				return true;
			}
			else if (jsonValue.IsUint64())
			{
				document.Add((long long)jsonValue.GetUint64());
				return true;
			}
			else if (jsonValue.IsDouble())
			{
				document.Add(jsonValue.GetDouble());
				return true;
			}
			else if (jsonValue.IsFloat())
			{
				document.Add(jsonValue.GetFloat());
				return true;
			}
			else if (jsonValue.IsObject())
			{
				Document obj;
				for (auto iter = jsonValue.MemberBegin(); iter != jsonValue.MemberEnd(); iter++)
				{
					const char *key = iter->name.GetString();
					if (!this->WriterToBson(key, obj, iter->value))
					{
						return false;
					}
				}

				document.Add(obj);
				return true;
			}
			else if (jsonValue.IsArray())
			{
				Bson::Writer::Array arrayDocument;
				for (int index = 0; index < jsonValue.Size(); index++)
				{
					if (!this->WriterToBson(arrayDocument, jsonValue[index]))
					{
						return false;
					}
				}
				document.Add(arrayDocument);
				return true;
			}
			return false;
		}

		bool Document::WriterToBson(const char *key, Document &document, const rapidjson::Value &jsonValue)
		{
			if (jsonValue.IsString())
			{
				const char *str = jsonValue.GetString();
				const size_t size = jsonValue.GetStringLength();
				document.Add(key, str, size);
				return true;
			}
			else if (jsonValue.IsBool())
			{
				document.Add(key, jsonValue.GetBool());
				return true;
			}
			else if (jsonValue.IsInt())
			{
				document.Add(key, jsonValue.GetInt());
				return true;
			}
			else if (jsonValue.IsInt64())
			{
				long long value = (long long)jsonValue.GetInt64();
				document.Add(key, value);
				return true;
			}
			else if (jsonValue.IsUint())
			{
				document.Add(key, jsonValue.GetUint());
				return true;
			}
			else if (jsonValue.IsUint64())
			{
				document.Add(key, (long long)jsonValue.GetUint64());
				return true;
			}
			else if (jsonValue.IsDouble())
			{
				document.Add(key, jsonValue.GetDouble());
				return true;
			}
			else if (jsonValue.IsFloat())
			{
				document.Add(key, jsonValue.GetFloat());
				return true;
			}
			else if (jsonValue.IsObject())
			{
				Document obj;
				for (auto iter = jsonValue.MemberBegin(); iter != jsonValue.MemberEnd(); iter++)
				{
					const char *key = iter->name.GetString();
					if (!this->WriterToBson(key, obj, iter->value))
					{
						return false;
					}
				}

				document.Add(key, obj);
				return true;
			}
			else if (jsonValue.IsArray())
			{
				Bson::Writer::Array arrayDocument;
				for (int index = 0; index < jsonValue.Size(); index++)
				{
					if (!this->WriterToBson(arrayDocument, jsonValue[index]))
					{
						return false;
					}
				}
				document.Add(key, arrayDocument);
				return true;
			}
			return false;
		}

		bool Document::WriterToStream(std::ostream &os)
		{
			const char *str = this->_done();
			const int size = this->len();
			os.write(str, size);
			return true;
		}

		int Document::GetStreamLength()
		{
			this->_done();
			return this->len();
		}

		const char *Document::Serialize(int &length)
		{
			char *bson = this->_done();
			length = this->len();
			return bson;
		}

		void Document::Add(const char *key, Array &document)
		{
			_bson::bsonobjbuilder &build = (_bson::bsonobjbuilder &)document;

			_b.appendNum((char)_bson::Array);
			_b.appendStr(key);

			const char * str = build._done();
			_b.appendBuf(str, build.len());
		}

		void Document::Add(const char *key, Document &document)
		{
			_bson::bsonobjbuilder &build = (_bson::bsonobjbuilder &)document;

			_b.appendNum((char)_bson::Object);
			_b.appendStr(key);
			const char * str = build._done();
			_b.appendBuf(str, build.len());
		}
	}
}
namespace Bson
{
	namespace Reader
	{
		bool Document::Get(const char *key, int &value) const
		{
			_bson::bsonelement bsonelement = this->getField(key);
			if (bsonelement.type() == _bson::BSONType::NumberInt)
			{
				value = bsonelement.Int();
				return true;
			}
			return false;
		}

		bool Document::Get(const char *key, bool &value) const
		{
			_bson::bsonelement bsonelement = this->getField(key);
			if (bsonelement.type() == _bson::BSONType::Bool)
			{
				value = bsonelement.Bool();
				return true;
			}
			return false;
		}

		bool Document::Get(const char *key, long long &value) const
		{
			_bson::bsonelement bsonelement = this->getField(key);
			switch (bsonelement.type())
			{
			case _bson::BSONType::NumberInt:
				value = bsonelement.Int();
				return true;
			case _bson::BSONType::NumberLong:
				value = bsonelement.Long();
				return true;
			}
			return false;
		}

		bool Document::Get(const char *key, std::string &value) const
		{
			_bson::bsonelement bsonelement = this->getField(key);
			switch (bsonelement.type())
			{
			case _bson::BSONType::String:
				value = bsonelement.String();
				return true;
			case _bson::BSONType::BinData:
			{
				int len = 0;
				const char *str = bsonelement.binData(len);
				value.append(str, len);
				return true;
			}
			}
			return false;
		}

		_bson::BSONType Document::Type(const char *key) const
		{
			_bson::bsonelement bsonelement = this->getField(key);
			return bsonelement.type();
		}

		void Document::WriterToJson(std::string &json)
		{
			Json::Writer jsonWriter;
			std::set<std::string> elements;
			this->getFieldNames(elements);
			for (const std::string &key : elements)
			{
				jsonWriter << key;
				this->WriterToJson(this->getField(key), jsonWriter);
			}
			jsonWriter.WriterStream(json);
		}

		void Document::WriterToJson(const _bson::bsonelement &bsonelement, Json::Writer &json)
		{
			switch (bsonelement.type())
			{
			case _bson::BSONType::Bool:
				json << bsonelement.Bool();
				break;
			case _bson::BSONType::String:
				json << bsonelement.String();
				break;
			case _bson::BSONType::NumberInt:
				json << bsonelement.Int();
				break;
			case _bson::BSONType::NumberLong:
				json << bsonelement.Long();
				break;
			case _bson::BSONType::NumberDouble:
				json << bsonelement.Double();
				break;
			case _bson::BSONType::BinData:
			{
				int len = 0;
				json.AddBinString(bsonelement.binData(len), len);
			}
			break;
			case _bson::BSONType::Object:
			{
				json.BeginObject();
				std::set<std::string> elements;
				_bson::bsonobj obj = bsonelement.object();
				obj.getFieldNames(elements);
				for (const std::string &key : elements)
				{
					json << key;
					this->WriterToJson(obj.getField(key), json);
				}
				json << Json::End::EndObject;
			}
			break;
			case _bson::BSONType::Array:
			{
				json.BeginArray();
				std::set<std::string> elements;
				const _bson::bsonobj obj = bsonelement.object();
				obj.getFieldNames(elements);
				for (const std::string &key : elements)
				{
					this->WriterToJson(obj.getField(key), json);
				}
				json << Json::End::EndArray;
			}
			break;
			case _bson::BSONType::Timestamp:
				json << bsonelement.timestampValue();
				break;
			case _bson::BSONType::Date:
				json << (long long)bsonelement.date().asInt64();
				break;
			default:
				json << bsonelement.toString();
				break;
			}
		}
	}
}