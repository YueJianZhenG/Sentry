

#ifndef APP_BSONDOCUMENT_H
#define APP_BSONDOCUMENT_H

#include "Proto/Bson/bsonobj.h"
#include "Yyjson/Document/Document.h"
#include "Proto/Bson/bsonobjbuilder.h"
#include "Proto/Bson/bsonobjiterator.h"
namespace bson
{

	namespace r
	{
		class Document;
		class Value
		{
		public:
			Value() = default;
			~Value() = default;
			explicit Value(_bson::bsonelement & element) : mElement(element) { }
		public:
			void Init(_bson::bsonelement & element) { this->mElement = element; }
			inline bool IsObject() const { return this->mElement.type() == _bson::BSONType::Object; }
		public:
			inline bool Get(bool & value) const
			{
				if(!this->mElement.isBoolean())
				{
					return false;
				}
				value = this->mElement.boolean();
				return true;
			}
			inline bool Get(std::string & value) const
			{
				if(this->mElement.type() != _bson::BSONType::String)
				{
					return false;
				}
				value = this->mElement.str();
				return true;
			}

			inline bool Get(const char * key, r::Value & value) const
			{
				if(this->mElement.type() != _bson::BSONType::Object)
				{
					return false;
				}
				value.mElement = this->mElement.object().getField(key);
				return true;
			}

			inline bool GetBool(const char * key, bool val = false) const
			{
				if(this->mElement.type() != _bson::BSONType::Object)
				{
					return false;
				}
				_bson::bsonelement element = this->mElement.object().getField(key);
				if(!element.isBoolean())
				{
					return val;
				}

				return element.Bool();
			}

			inline bool Get(const char * key, std::vector<r::Value> & value) const
			{
				if(this->mElement.type() != _bson::BSONType::Object)
				{
					return false;
				}
				_bson::bsonelement element = this->mElement.object().getField(key);
				if(element.type() != _bson::BSONType::Array)
				{
					return false;
				}
				for (_bson::bsonelement & item : element.Array())
				{
					value.emplace_back(item);
				}
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_integral<T>::value, bool> Get(const char * key, T & value) const
			{
				if(this->mElement.type() != _bson::BSONType::Object)
				{
					return false;
				}
				_bson::bsonelement element = this->mElement.object().getField(key);
				switch(element.type())
				{
					case _bson::BSONType::NumberInt:
					{
						value = (T)element.Int();
						return true;
					}
					case _bson::BSONType::NumberLong:
					{
						value = (T)element.Long();
						return true;
					}
					default:
						return false;
				}
			}

			template<typename T>
			inline std::enable_if_t<std::is_floating_point<T>::value, bool> Get(const char * key, T & value) const
			{
				if(this->mElement.type() != _bson::BSONType::Object)
				{
					return false;
				}
				_bson::bsonelement element = this->mElement.object().getField(key);
				if(element.type() != _bson::BSONType::NumberDouble)
				{
					return false;
				}
				value = (T)element.Double();
				return true;
			}

			inline bool Get(const char * key, std::string & value) const
			{
				if(this->mElement.type() != _bson::BSONType::Object)
				{
					return false;
				}
				_bson::bsonelement element = this->mElement.object().getField(key);
				if(element.type() != _bson::BSONType::String)
				{
					return false;
				}
				value = element.String();
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_integral<T>::value, bool> Get(T& v) const
			{
				switch(this->mElement.type())
				{
					case _bson::BSONType::NumberInt:
					{
						v = (T)this->mElement.Int();
						return true;
					}
					case _bson::BSONType::NumberLong:
					{
						v = (T)this->mElement.Long();
						return true;
					}
				}
				return false;
			}

			template<typename T>
			inline std::enable_if_t<std::is_floating_point<T>::value, bool> Get(T& v) const
			{
				if(this->mElement.type() != _bson::BSONType::NumberDouble)
				{
					return false;
				}
				v = (T)this->mElement.Double();
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_floating_point<T>::value, bool> Get(std::vector<T>& v) const
			{
				if(this->mElement.type() != _bson::BSONType::Array)
				{
					return false;
				}
				for(const _bson::bsonelement & element : this->mElement.Array())
				{
					if(element.type() != _bson::BSONType::NumberDouble)
					{
						return false;
					}
					v.emplace_back((T)element.Double());
				}
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_integral<T>::value, bool> Get(std::vector<T>& v) const
			{
				if(this->mElement.type() != _bson::BSONType::Array)
				{
					return false;
				}
				for(const _bson::bsonelement & element : this->mElement.Array())
				{
					if(element.type() == _bson::BSONType::NumberInt)
					{
						v.emplace_back((T)element.Int());
					}
					else if(element.type() == _bson::BSONType::NumberLong)
					{
						v.emplace_back((T)element.Long());
					}
					else
					{
						return false;
					}
				}
				return true;
			}

			inline _bson::BSONType Type() const { return this->mElement.type(); }
			inline _bson::BSONType Type(const char * key) const {
				if(this->mElement.type() != _bson::BSONType::Object)
				{
					return _bson::BSONType::EOO;
				}
				return this->mElement.object().getField(key).type();
			}
			inline std::string ToString() { return this->mElement.toString(); }
			inline _bson::bsonelement & ToElement() { return this->mElement; }
		private:
			_bson::bsonelement mElement;
		};

		class Document
		{
		public:
			explicit Document() = default;
			explicit Document(_bson::bsonobj & object) : mObject(object) { }
			explicit Document(const char * bson) { this->mObject.init(bson); }
			explicit Document(_bson::bsonelement & element) : mObject(element.object()) { }
		public:
			inline bool IsOk() const {
				double isOk = 0;
				return this->Get("ok", isOk) && isOk != 0;
			}
			bool GetFirst(Document & document) const;
			inline void Init(const char * bson) { this->mObject.init(bson); }
			inline bool Get(const char * key, r::Value & value) const
			{
				_bson::bsonelement element = this->mObject.getField(key);
				if(element.eoo())
				{
					return false;
				}
				value.Init(element);
				return true;
			}

			inline bool GetBool(const char * key, bool val = false) const
			{
				_bson::bsonelement element = this->mObject.getField(key);
				if(!element.isBoolean())
				{
					return val;
				}

				return element.Bool();
			}

			inline bool Get(const char * key, std::list<r::Value> & value) const
			{
				_bson::bsonelement element = this->mObject.getField(key);
				if(element.type() != _bson::BSONType::Array)
				{
					return false;
				}
				for (_bson::bsonelement & item : element.Array())
				{
					value.emplace_back(item);
				}
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_integral<T>::value, bool> Get(const char * key, T & value) const
			{
				_bson::bsonelement element = this->mObject.getField(key);
				switch(element.type())
				{
					case _bson::BSONType::NumberInt:
					{
						value = (T)element.Int();
						return true;
					}
					case _bson::BSONType::NumberLong:
					{
						value = (T)element.Long();
						return true;
					}
					default:
						return false;
				}
			}

			template<typename T>
			inline std::enable_if_t<std::is_floating_point<T>::value, bool> Get(const char * key, T & value) const
			{
				_bson::bsonelement element = this->mObject.getField(key);
				if(element.type() != _bson::BSONType::NumberDouble)
				{
					return false;
				}
				value = (T)element.Double();
				return true;
			}


			inline bool Get(const char * key, std::string & value) const
			{
				_bson::bsonelement element = this->mObject.getField(key);
				if(element.type() != _bson::BSONType::String)
				{
					return false;
				}
				value = element.String();
				return true;
			}

			inline bool Get(const char * key, r::Document & value) const
			{
				_bson::bsonelement element = this->mObject.getField(key);
				if(element.type() != _bson::BSONType::Object)
				{
					return false;
				}
				value.mObject = element.object();
				return true;
			}

			inline bool Get(const char * key, std::list<r::Document> & value) const
			{
				_bson::bsonelement element = this->mObject.getField(key);
				if(element.type() != _bson::BSONType::Array)
				{
					return false;
				}
				for(_bson::bsonelement & item : element.Array())
				{
					if(item.type() != _bson::BSONType::Object)
					{
						return false;
					}
					value.emplace_back(item);
				}
				return true;
			}
		public:
			bool WriterToJson(std::string * json);
			bool WriterToJson(json::w::Document & document);
			inline _bson::bsonobj & ToObject() { return this->mObject; }
			inline std::string ToString() { return this->mObject.toString(); }
		private:
			void WriterToJson(const _bson::bsonelement& bsonelement, json::w::Value& json);
		private:
			_bson::bsonobj mObject;
		};
	}


	namespace w
	{
		class Document : protected _bson::bsonobjbuilder
		{
		public:
			Document(w::Document & document);
			explicit Document(bool object = true);
			~Document() final = default;
		public:
			std::string ToString();
			bool FromByJson(const std::string& json);
			bool FromByJson(const json::r::Value & json);
			bool FromByJson(const char * json, size_t size);
			inline bool IsArray() const { return this->mType == _bson::BSONType::Array; }
			inline bool IsObject() const { return this->mType == _bson::BSONType::Object; }
		public:
			template<typename T>
			inline bool Push(const T & value) {
				if(this->mType != _bson::BSONType::Array)
				{
					return false;
				}
				this->append(std::to_string(this->mIndex++), value);
				return true;
			}
			inline bool Push(w::Document & document) {
				if(this->mType != _bson::BSONType::Array)
				{
					return false;
				}
				_bson::bsonobjbuilder &build = (_bson::bsonobjbuilder &)document;
				_b.appendNum((char)document.mType);
				_b.appendStr(std::to_string(this->mIndex++));

				const char * str = build._done();

				_b.appendBuf(str, build.len());
				return true;
			}

			inline bool Push(const json::r::Value & document) {
				if(this->mType != _bson::BSONType::Array)
				{
					return false;
				}
				w::Document document1;
				if(!document1.FromByJson(document))
				{
					return false;
				}
				return this->Push(document1);
			}

			inline bool PushObject(const std::string & json) {
				if(this->mType != _bson::BSONType::Array)
				{
					return false;
				}
				w::Document document1;
				if(!document1.FromByJson(json))
				{
					return false;
				}
				return this->Push(document1);
			}

		public:
			const char* Serialize(int& length);
		public:
			template<typename T>
			inline bool AddObject(const char * key, const char * k1, T & value)
			{
				w::Document val;
				val.Add(k1, value);
				return this->Add(key, val);
			}
		public:
			bool Add(const char * key);
			bool Add(const char* key, r::Value& document);
			bool Add(const char* key, r::Document& document);
			bool Add(const char* key, w::Document& document);
			bool Add(const char* key, const json::r::Value& document);
			bool AddObject(const char* key, const std::string & json);


			template<typename T>
			inline bool Add(const char * key, const T & value)
			{
				if(this->mType != _bson::BSONType::Object)
				{
					return false;
				}
				this->append(key, value);
				return true;
			}

			inline bool Add(const char* key, const char* str, size_t size)
			{
				if(this->mType != _bson::BSONType::Object)
				{
					return false;
				}
				this->append(key, str, size + 1);
				return true;
			}
		private:
			bool WriterToBson(Document& document, const json::r::Value& jsonValue);
			bool WriterToBson(const char* key, Document& document, const json::r::Value& jsonValue);
		private:
			unsigned int mIndex;
			_bson::BSONType mType;
		};
	}

}


#endif //APP_BSONDOCUMENT_H
