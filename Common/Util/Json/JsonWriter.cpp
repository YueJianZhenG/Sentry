//
// Created by yjz on 2022/3/26.
//
#include"JsonWriter.h"

namespace Json
{
	Writer& Writer::BeginArray(const char* key)
	{
		this->mWriter->String(key);
		this->mWriter->StartArray();
		return *this;
	}

	Writer& Writer::BeginArray()
	{
		this->mWriter->StartArray();
		return *this;
	}

	Writer& Writer::BeginObject()
	{
		this->mWriter->StartObject();
		return *this;
	}

	Writer& Writer::BeginObject(const char* key)
	{
		this->mWriter->String(key);
		this->mWriter->StartObject();
		return *this;
	}

	Writer& Writer::operator<<(Json::End type)
	{
		switch(type)
		{
		case End::EndObject:
			this->mWriter->EndObject();
			break;
		case End::EndArray:
			this->mWriter->EndArray();
			break;
		}
		return *this;
	}

	Writer& Writer::operator<<(std::vector<int>& value)
	{
		for(const int val : value)
		{
			this->mWriter->Int(val);
		}
		this->mWriter->EndArray();
		return *this;
	}

	Writer& Writer::operator<<(std::vector<std::string>& value)
	{
		for(const std::string & val : value)
		{
			this->mWriter->String(val.c_str(), val.size());
		}
		return *this;
	}

	Writer& Writer::operator<<(std::list<std::string>& value)
	{
		for(const std::string & val : value)
		{
			this->mWriter->String(val.c_str(), val.size());
		}
		return *this;
	}
}

namespace Json
{
	Writer::Writer(bool isObj)
		: mIsObject(isObj)
	{
        this->mStringBuf = new rapidjson::StringBuffer();
        this->mWriter = new  rapidjson::Writer<rapidjson::StringBuffer>(*this->mStringBuf);
        //this->mDocument.Accept(this->mWriter);
		if (this->mIsObject)
		{
			this->mWriter->StartObject();
			return;
		}
		this->mWriter->StartArray();
	}

    Writer::~Writer()
    {
        delete this->mWriter;
        delete this->mStringBuf;
    }



	const std::string Writer::JsonString()
	{
		if (this->mWriter->IsComplete())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			return std::string(str, size);
		}
		if (this->mIsObject && this->mWriter->EndObject())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			return std::string(str, size);
		}
		else if (this->mWriter->EndArray())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			return std::string(str, size);
		}
		return std::string();
	}

	size_t Writer::WriterStream(std::string& os)
	{
		if (this->mWriter->IsComplete())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			os.append(str, size);
			return size;
		}
		if (this->mIsObject && this->mWriter->EndObject())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			os.append(str, size);
			return size;
		}
		else if (this->mWriter->EndArray())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			os.append(str, size);
			return size;
		}
		return 0;
	}

	size_t Writer::WriterStream(std::ostream& os)
	{
		if (this->mWriter->IsComplete())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			os.write(str, size);
			return size;
		}
		if (this->mIsObject && this->mWriter->EndObject())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			os.write(str, size);
			return size;
		}
		else if (this->mWriter->EndArray())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			os.write(str, size);
			return size;
		}
		return 0;
	}
	size_t Writer::GetJsonSize()
	{
		if (this->mWriter->IsComplete())
		{
			return this->mStringBuf->GetSize();
		}
		if (this->mIsObject && this->mWriter->EndObject())
		{
			return this->mStringBuf->GetSize();
		}
		else if (this->mWriter->EndArray())
		{
			return this->mStringBuf->GetSize();
		}
		return 0;
	}

	void Writer::AddBinString(const char* str, size_t size)
	{
        rapidjson::Document document;
        document.AddMember("1122", document, document.GetAllocator());
		this->mWriter->String(str, size);
	}


	bool Writer::GetDocument(rapidjson::Document& jsonDocument)
	{
		if (this->mWriter->IsComplete())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			return !jsonDocument.Parse(str, size).HasParseError();
		}
		if (this->mIsObject && this->mWriter->EndObject())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			return !jsonDocument.Parse(str, size).HasParseError();
		}
		else if (this->mWriter->EndArray())
		{
			const char* str = this->mStringBuf->GetString();
			const size_t size = this->mStringBuf->GetSize();
			return !jsonDocument.Parse(str, size).HasParseError();
		}
		return false;
	}
}

namespace Json
{
    Document::Document(const std::string &json)
    {
        this->Parse(json.c_str(), json.size());
    }

    Document::Document(const char *json, size_t size)
    {
        this->Parse(json, size);
    }

    Document &Document::Add(const char *key, int value)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, value, this->GetAllocator());
        return *this;
    }

    Document &Document::Add(const char *key, bool value)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, value, this->GetAllocator());
        return *this;
    }

    Document &Document::Add(const char *key, long long value)
    {
        rapidjson::GenericStringRef<char> name(key);
#ifndef __OS_LINUX__
        this->AddMember(name, value, this->GetAllocator());
#else
        this->AddMember(name, (int64_t)value, this->GetAllocator());
#endif
        return *this;
    }

    Document &Document::Add(const char *key, const char *value)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, rapidjson::StringRef(value),
                                  this->GetAllocator());
        return *this;
    }

    Document &Document::Add(const char *key, const char *value, size_t size)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, rapidjson::StringRef(value, size),
                                  this->GetAllocator());
        return *this;
    }

    Document &Document::Add(const char *key, const std::string &value)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, rapidjson::StringRef(
            value.c_str(), value.size()),this->GetAllocator());
        return *this;
    }

    Document &Document::Add(const char *key, unsigned int value)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, value, this->GetAllocator());
        return *this;
    }

    Document &Document::Add(const char *key, float value)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, value, this->GetAllocator());
        return *this;
    }

    Document &Document::Add(const char *key, double value)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, value, this->GetAllocator());
        return *this;
    }

    Document &Document::Add(const char *key, unsigned long long value)
    {
        rapidjson::GenericStringRef<char> name(key);
#ifndef __OS_LINUX__
        this->AddMember(name, value, this->GetAllocator());
#else
        this->AddMember(name, (uint64_t)value, this->GetAllocator());
#endif
        return *this;
    }

    Document &Document::Add(const char *key, Document &value)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, (rapidjson::Document&)value, this->GetAllocator());
        return *this;
    }

    Document &Document::Add(const char *key, rapidjson::Document &value)
    {
        rapidjson::GenericStringRef<char> name(key);
        this->AddMember(name, value, this->GetAllocator());
        return *this;
    }

    std::string * Document::Serialize(std::string *json)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        this->Accept(writer);
        json->append(buffer.GetString(), buffer.GetLength());
        return json;
    }

    const rapidjson::Value *Document::Get(const char *key) const
    {
        if(!this->IsObject())
        {
            return nullptr;
        }
        auto iter = this->FindMember(key);
        if(iter == this->MemberEnd())
        {
            return nullptr;
        }
        return &iter->value;
    }

    const rapidjson::Value *Document::Get(const char *k1, const char *k2) const
    {
        const rapidjson::Value * value = this->Get(k1);
        if(value == nullptr || !value->IsObject())
        {
            return nullptr;
        }
        auto iter = value->FindMember(k2);
        if(iter == value->MemberEnd())
        {
            return nullptr;
        }
        return &iter->value;
    }
}