//
// Created by leyi on 2023/11/17.
//

#ifndef APP_DOCUMENT_H
#define APP_DOCUMENT_H

#include <list>
#include <string>
#include <memory>
#include <vector>

#include <functional>
#include <unordered_map>
#include "Core/Wrap/Wrap.h"
#include "Yyjson/Src/yyjson.h"

namespace json
{

	namespace r
	{
		class Value;
	}
	namespace w
	{
		class Value;
	}

	class IObject
	{
	public:
		virtual bool Encode(json::w::Value & document) = 0;
		virtual bool Decode(const json::r::Value & document) = 0;
	};


	namespace w
	{
		class Value
		{
		public:
			Value(yyjson_mut_doc* doc, yyjson_mut_val* val);

			virtual ~Value() = default;

		protected:
			Value();
		public:
			bool Push(const char *);
			bool Push(const std::string& v);
			bool Push(r::Value& value);
			bool Push(const w::Value& value);
			bool Push(const char* str, size_t size);
		public:
			bool PushObject(const std::string& str);
			bool PushObject(const std::list<std::string>& list);
			bool PushObject(r::Value & value, std::unique_ptr<w::Value> & result);
			bool PushObject(const std::string& str, std::unique_ptr<Value> & value);

			template<typename T>
			inline std::enable_if_t<std::is_base_of<IObject, T>::value, bool> Push(T & object)
			{
				if(!this->IsArray())
				{
					return false;
				}
				auto jsonObject = this->AddObject();
				return object.Encode(*jsonObject);
			}

			template<typename T>
			inline std::enable_if_t<std::is_integral<T>::value, bool> Push(T v)
			{
				if (!this->IsArray())
				{
					return false;
				}
				yyjson_mut_val* val = yyjson_mut_int(this->mDoc, v);
				return yyjson_mut_arr_add_val(this->mValue, val);
			}
			template<typename T>
			inline std::enable_if_t<std::is_floating_point<T>::value, bool> Push(T v)
			{
				if (!this->IsArray())
				{
					return false;
				}
				yyjson_mut_val* val = yyjson_mut_real(this->mDoc, v);
				return yyjson_mut_arr_add_val(this->mValue, val);
			}
		public:
			bool AddNull(const char* key);

			template<typename T>
			inline std::enable_if_t<std::is_integral<T>::value, bool> Add(const char* k, T v)
			{
				if (!this->IsObject())
				{
					return false;
				}
				return yyjson_mut_obj_add_int(this->mDoc, this->mValue, k, v);
			}

			template<typename T>
			inline std::enable_if_t<std::is_enum<T>::value, bool> Add(const char* k, T v)
			{
				if (!this->IsObject())
				{
					return false;
				}
				return yyjson_mut_obj_add_int(this->mDoc, this->mValue, k, (int)v);
			}

			template<typename T>
			inline std::enable_if_t<std::is_floating_point<T>::value, bool> Add(const char* k, T v)
			{
				if (!this->IsObject())
				{
					return false;
				}
				yyjson_mut_val * val = yyjson_mut_real(this->mDoc, v);
				yyjson_mut_val * key = yyjson_mut_strcpy(this->mDoc, k);
				return yyjson_mut_obj_add(this->mValue, key, val);
			}

			bool Add(const char* k, char v);
			bool Add(const char* k, bool v);
			bool Add(const char* k, const char * v);
			bool Add(const char* k, yyjson_val* v);
			bool Add(const char* k, yyjson_mut_val* v);
			bool Add(const char* k, r::Value& value);
			bool Add(const char* k, const w::Value& value);
			bool Add(const char* k, const std::string& v);
			bool Add(const char* k, const char* str, size_t size);
			bool Add(const char* k, const std::list<std::string> & value);

			template<typename T>
			inline std::enable_if_t<std::is_base_of<IObject, T>::value, bool> Add(const char* k, T & value)
			{
				auto jsonObject = this->AddObject(k);
				return value.Encode(*jsonObject);
			}


			template<typename T>
			inline bool Add(const char* k, const std::vector<T>& value, bool addEmpty = true)
			{
				if (!this->IsObject())
				{
					return false;
				}
				if(!addEmpty && value.empty())
				{
					return true;
				}
				std::unique_ptr<Value> jsonArray = this->AddArray(k);
				for (const T& val: value)
				{
					jsonArray->Push(val);
				}
				return true;
			}
		public:
			bool AddObject(const char* json, size_t size);
			bool AddObject(const char* key, const std::string& json);
			bool AddObject(const char* key, const char* json, size_t size);
		public:
			std::unique_ptr<Value> AddArray();
			std::unique_ptr<Value> AddObject();
			std::unique_ptr<Value> AddArray(const char* k);
			std::unique_ptr<Value> AddObject(const char* k);
		public:
			inline yyjson_mut_doc * GetDocument() { return this->mDoc; }
			inline bool IsInt() const { return yyjson_mut_is_int(this->mValue);}
			inline bool IsArray() const { return yyjson_mut_is_arr(this->mValue); }
			inline bool IsObject() const { return yyjson_mut_is_obj(this->mValue); }

		protected:
			yyjson_mut_doc* mDoc;
			yyjson_mut_val* mValue;
		};

		class Document final : public Value
		{
		public:
			explicit Document(bool array = false);
			explicit Document(const std::string& json);
			Document(yyjson_mut_doc* doc, yyjson_mut_val* val);
			~Document() final { yyjson_mut_doc_free(this->mDoc); }
		public:
			std::string JsonString(bool pretty = false) const;
			bool Deserialization(const char * json, size_t size);
			bool Serialize(std::string * json, bool pretty = false, yyjson_write_flag = 0) const noexcept;
			bool Serialize(wrap::string<true> & json, bool pretty = false, yyjson_write_flag = 0) const noexcept;
		private:
		};
	}

	namespace r
	{
		class Value
		{
		public:
			Value() : mValue(nullptr) { }
			explicit Value(yyjson_val* val) : mValue(val) { }

			virtual ~Value() = default;
		public:

			template<typename T>
			inline std::enable_if_t<std::is_integral<T>::value, bool> Get(T& v) const
			{
				if (!yyjson_is_num(this->mValue))
				{
					return false;
				}
				v = yyjson_get_sint(this->mValue);
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_floating_point<T>::value, bool> Get(T& v) const
			{
				if (!yyjson_is_real(this->mValue))
				{
					return false;
				}
				v = yyjson_get_real(this->mValue);
				return true;
			}
		public:
			bool Get(bool& v) const;
			bool Get(std::string& v) const;
			bool Get(size_t index, int& value) const;
			bool Get(size_t index, std::string& value) const;
			int GetInt(const char* k, int defaultVal = 0) const;
			bool GetBool(const char * key, bool defaultVal = false) const;
			bool GetFirst(std::string & key, json::r::Value& value) const;
			bool Get(size_t index, r::Value & value) const;
			bool Foreach(std::function<void(const char * key, r::Value & value)> && callback);
		public:
			const char * GetString(size_t & size) const;
			const char * GetString(size_t index, size_t & size) const;
			const char * GetString(const char * key, size_t & size) const;
		public:
			size_t MemberCount() const;
			std::string ToString() const;
			std::vector<const char *> GetAllKey() const;
			size_t GetKeys(std::vector<const char*>& key) const;
			inline yyjson_val* GetValue() { return this->mValue; }
			bool ToCString(std::unique_ptr<char> & json, size_t & size) const;
		public:
			bool Get(const char* k, bool& v) const;
			bool Get(const char* k, std::string& v) const;
			bool GetValues(std::vector<r::Value> & value) const;
		public:
			bool Get(const char* key, std::vector<r::Value> & value) const;
		public:
			bool Get(const char* key, r::Value & value) const;
			bool Get(const char* key, std::vector<std::string>& value) const;
			bool Get(const char * key, std::unordered_map<int, std::string> & value) const;
			bool Get(const char * key, std::unordered_map<std::string, std::string> & value) const;
			template<typename T>
			inline bool Get(const char * k1, const char * k2, T & value)
			{
				yyjson_val* val = yyjson_obj_get(this->mValue, k1);
				if(!yyjson_is_obj(val))
				{
					return false;
				}
				json::r::Value value1(val);
				return value1.Get(k2, value);
			}


			template<typename T>
			inline std::enable_if_t<std::is_base_of<IObject, T>::value, bool> Get(const char * key, T & value)
			{
				yyjson_val* val = yyjson_obj_get(this->mValue, key);
				if (val == nullptr)
				{
					return false;
				}
				Value jsonValue(val);
				return value.Decode(jsonValue);
			}

			template<typename T>
			inline std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value, bool> Get(const char* k, T& v) const
			{
				yyjson_val* val = yyjson_obj_get(this->mValue, k);
				if (!yyjson_is_int(val))
				{
					return false;
				}
				v = (T)yyjson_get_sint(val);
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_floating_point<T>::value, bool> Get(const char* k, T& v) const
			{
				yyjson_val* val = yyjson_obj_get(this->mValue, k);
				if (!yyjson_is_int(val))
				{
					return false;
				}
				v = (T)yyjson_get_num(val);
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_integral<T>::value, bool> Get(const char* key, std::vector<T>& value) const
			{
				yyjson_val* jsonArr = yyjson_obj_get(this->mValue, key);
				if (!yyjson_is_arr(jsonArr))
				{
					return false;
				}
				size_t size = yyjson_arr_size(jsonArr);
				value.reserve(size);
				for (size_t index = 0; index < size; index++)
				{
					yyjson_val* item = yyjson_arr_get(jsonArr, index);
					if (!yyjson_is_num(item))
					{
						return false;
					}
					else
					{
						value.emplace_back((T)yyjson_get_sint(item));
					}
				}
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_base_of<IObject, T>::value, bool> Get(const char* key, std::vector<T>& value) const
			{
				yyjson_val* jsonArr = yyjson_obj_get(this->mValue, key);
				if (!yyjson_is_arr(jsonArr))
				{
					return false;
				}
				T newItem;
				size_t size = yyjson_arr_size(jsonArr);
				value.reserve(size);
				for (size_t index = 0; index < size; index++)
				{
					yyjson_val* item = yyjson_arr_get(jsonArr, index);
					if (!yyjson_is_obj(item))
					{
						return false;
					}
					json::r::Value jsonVal(item);
					if(!newItem.Decode(jsonVal))
					{
						return false;
					}
					value.emplace_back(newItem);
				}
				return true;
			}

			template<typename T>
			inline std::enable_if_t<std::is_floating_point<T>::value, bool> Get(const char* key, std::vector<T>& value) const
			{
				yyjson_val* jsonArr = yyjson_obj_get(this->mValue, key);
				if (!yyjson_is_arr(jsonArr))
				{
					return false;
				}
				size_t size = yyjson_arr_size(jsonArr);
				value.reserve(size);
				for (size_t index = 0; index < size; index++)
				{
					yyjson_val* item = yyjson_arr_get(jsonArr, index);
					if (!yyjson_is_num(item))
					{
						return false;
					}
					value.emplace_back((T)yyjson_get_num(item));
				}
				return true;
			}
		public:
			inline bool IsArray() const { return yyjson_is_arr(this->mValue); }
			inline bool IsObject() const { return yyjson_is_obj(this->mValue); }
			inline int GetType() const { return yyjson_get_type(this->mValue); }
			inline int GetSubType() const { return yyjson_get_subtype(this->mValue); }
		protected:
			yyjson_val* mValue;
		};

		class Document : public r::Value
		{
		public:
			explicit Document() : Value(nullptr), mDoc(nullptr) { }
			explicit Document(w::Value & document);
			~Document() override  { yyjson_doc_free(this->mDoc); }

		public:
			void SetDoc(yyjson_doc* doc);
			yyjson_doc* GetDoc() { return this->mDoc; }
			bool FromFile(const std::string& path) noexcept;
			// 启用YYJSON_READ_INSITU之后 会改变原始字符串
			bool Decode(const std::string& json, yyjson_read_flag flag = 0) noexcept;
			bool Decode(const char* str, size_t size, yyjson_read_flag flag = 0) noexcept;
			bool DecodeFile(const std::string& path);
		public:
			inline const std::string& GetError()
			{
				return this->mError;
			}

		private:
			yyjson_doc* mDoc;
			std::string mError;
		};
	}

	extern void Merge(json::w::Document& target, json::r::Value& source);
}


#endif //APP_DOCUMENT_H
