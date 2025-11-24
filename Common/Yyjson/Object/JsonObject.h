//
// Created by 64658 on 2024/8/27.
//

#ifndef APP_JSONOBJECT_H
#define APP_JSONOBJECT_H
#include <map>
#include "Yyjson/Document/Document.h"

namespace json
{
    class ValueBase
    {
    public:
        ValueBase() = default;
        virtual ~ValueBase() = default;
        virtual bool Set(json::w::Value& document, const std::string& key, void* obj) = 0;
        virtual bool Get(const json::r::Value& document, const std::string& key, void* obj) const = 0;

    public:
        std::string name;
    };

    template <typename T, typename V>
    class ValueProxy : public ValueBase
    {
    public:
        ValueProxy(V T::* member, bool must) : field(member), mMust(must)
        {
        }

    public:
        bool Set(const V& value, void* obj);
        bool Set(json::w::Value& document, const std::string& key, void* obj) final;
        bool Get(const json::r::Value& document, const std::string& key, void* obj) const final;

    private:
        bool mMust;
        V T::* field;
    };

    template <typename T, typename V>
    class ObjectValueProxy : public ValueBase
    {
    public:
        ObjectValueProxy(V T::* member, bool must) : field(member), mMust(must)
        {
        }

    public:
        bool Set(json::w::Value& document, const std::string& key, void* obj) final;
        bool Get(const json::r::Value& document, const std::string& key, void* obj) const final;

    private:
        bool mMust;
        V T::* field;
    };


    template <typename T, typename V>
    class ObjectArrayValueProxy : public ValueBase
    {
    public:
        ObjectArrayValueProxy(std::vector<V> T::* member, bool must) : field(member), mMust(must)
        {
        }

    public:
        bool Set(json::w::Value& document, const std::string& key, void* obj) final;
        bool Get(const json::r::Value& document, const std::string& key, void* obj) const final;

    private:
        bool mMust;
        std::vector<V> T::* field;
    };

    template <typename T, typename V>
    class ObjectMapValueProxy : public ValueBase
    {
    public:
        ObjectMapValueProxy(std::unordered_map<std::string, V> T::* member, bool must) : field(member), mMust(must)
        {
        }

    public:
        bool Set(json::w::Value& document, const std::string& key, void* obj) final;
        bool Get(const json::r::Value& document, const std::string& key, void* obj) const final;

    private:
        bool mMust;
        std::unordered_map<std::string, V> T::* field;
    };

    template <typename T, typename V>
    class MapValueProxy : public ValueBase
    {
    public:
        MapValueProxy(std::unordered_map<std::string, V> T::* member, bool must) : field(member), mMust(must)
        {
        }

    public:
        bool Set(json::w::Value& document, const std::string& key, void* obj) final;
        bool Get(const json::r::Value& document, const std::string& key, void* obj) const final;

    private:
        bool mMust;
        std::unordered_map<std::string, V> T::* field;
    };

    template <typename T, typename V>
    inline bool ValueProxy<T, V>::Set(const V& value, void* obj)
    {
        T* inst = (T*)obj;
        (inst->*field) = value;
        return true;
    }

    template <typename T, typename V>
    inline bool ValueProxy<T, V>::Set(json::w::Value& document, const std::string& key, void* obj)
    {
        T* inst = (T*)obj;
        return document.Add(key.c_str(), inst->*field);
    }


    template <typename T, typename V>
    inline bool ValueProxy<T, V>::Get(const json::r::Value& document, const std::string& key, void* obj) const
    {
        T* inst = (T*)obj;
        return document.Get(key.c_str(), inst->*field) || !this->mMust;
    }

    template <typename T>
    class Object : public IObject
    {
    public:
        explicit Object() = default;
        virtual ~Object() = default;

    public:
        bool Encode(json::w::Value& document) final;
        bool Decode(const json::r::Value& document) final;

        template <typename V>
        inline bool Set(const std::string& k, const V& value)
        {
            auto iter = values.find(k);
            if (iter == values.end())
            {
                return false;
            }
            ValueProxy<T, V>* valueProxy = dynamic_cast<ValueProxy<T, V>*>(iter->second.get());
            if (valueProxy == nullptr)
            {
                return false;
            }
            valueProxy->Set(value, this);
            return true;
        }

    protected:
        virtual bool OnDecodeOk() { return true; }

    public:
        static std::unique_ptr<T> Create(const std::string& json)
        {
            json::r::Document document;
            if (!document.Decode(json))
            {
                return nullptr;
            }
            std::unique_ptr<T> result = std::make_unique<T>();
            {
                if (!result->Decode(document))
                {
                    return nullptr;
                }
            }
            return result;
        }

    public:
        template <typename V>
        static std::enable_if_t<std::is_base_of<IObject, V>::value, bool>
        RegisterField(const char* name, std::vector<V> T::* member, bool must = false)
        {
            auto iter = values.find(name);
            if (iter != values.end())
            {
                return false;
            }
            values.emplace(name, std::make_unique<ObjectArrayValueProxy<T, V>>(member, must));
            return true;
        }

        template <typename V>
        static std::enable_if_t<std::is_base_of<IObject, V>::value, bool>
        RegisterField(const char* name, std::unordered_map<std::string, V> T::* member, bool must = false)
        {
            auto iter = values.find(name);
            if (iter != values.end())
            {
                return false;
            }
            values.emplace(name, std::make_unique<ObjectMapValueProxy<T, V>>(member, must));
            return true;
        }

        template <typename V>
        static std::enable_if_t<!std::is_base_of<IObject, V>::value, bool>
        RegisterField(const char* name, std::unordered_map<std::string, V> T::* member, bool must = false)
        {
            auto iter = values.find(name);
            if (iter != values.end())
            {
                return false;
            }
            values.emplace(name, std::make_unique<MapValueProxy<T, V>>(member, must));
            return true;
        }


        template <typename V>
        static std::enable_if_t<!std::is_base_of<IObject, V>::value, bool>
        RegisterField(const char* name, V T::* member, bool must = false)
        {
            auto iter = values.find(name);
            if (iter != values.end())
            {
                return false;
            }
            values.emplace(name, std::make_unique<ValueProxy<T, V>>(member, must));
            return true;
        }

        template <typename V>
        static std::enable_if_t<std::is_base_of<IObject, V>::value, bool>
        RegisterField(const char* name, V T::* member, bool must = false)
        {
            auto iter = values.find(name);
            if (iter != values.end())
            {
                return false;
            }
            values.emplace(name, std::make_unique<ObjectValueProxy<T, V>>(member, must));
            return true;
        }

    private:
        static std::unordered_map<std::string, std::unique_ptr<ValueBase>> values;
    };

    template <typename T>
    static std::unique_ptr<T> Create(const std::string& json)
    {
        json::r::Document document;
        if (!document.Decode(json))
        {
            return nullptr;
        }
        std::unique_ptr<T> data = std::make_unique<T>();
        if (!data->Decode(document))
        {
            return nullptr;
        }
        return data;
    }

    template <typename T>
    static std::unique_ptr<T> Create(json::r::Value& document)
    {
        std::unique_ptr<T> data = std::make_unique<T>();
        if (!data->Decode(document))
        {
            return nullptr;
        }
        return data;
    }

    template <typename T>
    inline bool Object<T>::Decode(const json::r::Value& document)
    {
        for (auto iter = values.begin(); iter != values.end(); iter++)
        {
            const std::string& key = iter->first;
            if (!iter->second->Get(document, key, this))
            {
                return false;
            }
        }
        return this->OnDecodeOk();
    }

    template <typename T>
    inline bool Object<T>::Encode(json::w::Value& document)
    {
        auto iter = values.begin();
        for (; iter != values.end(); iter++)
        {
            const std::string& key = iter->first;
            iter->second->Set(document, key, this);
        }
        return true;
    }

    template <typename T, typename V>
    inline bool ObjectValueProxy<T, V>::Set(json::w::Value& document, const std::string& key, void* obj)
    {
        std::unique_ptr<json::w::Value> jsonValue = document.AddObject(key.c_str());
        {
            T* inst = (T*)obj;
            return (inst->*field).Encode(*jsonValue);
        }
    }

    template <typename T, typename V>
    inline bool ObjectValueProxy<T, V>::Get(const json::r::Value& document, const std::string& key, void* obj) const
    {
        T* inst = (T*)obj;
        json::r::Value jsonValue;
        if (!document.Get(key.c_str(), jsonValue))
        {
            if (!this->mMust)
            {
                return true;
            }
            return false;
        }
        return (inst->*field).Decode(jsonValue);
    }

    template <typename T, typename V>
    inline bool ObjectArrayValueProxy<T, V>::Set(json::w::Value& document, const std::string& key, void* obj)
    {
        T* inst = (T*)obj;
        std::unique_ptr<json::w::Value> jsonArray = document.AddArray(key.c_str());
        {
            for (V& val : (inst->*field))
            {
                std::unique_ptr<json::w::Value> jsonObject = jsonArray->AddObject();
                if (!val.Encode(*jsonObject))
                {
                    return false;
                }
            }
        }
        return true;
    }

    template <typename T, typename V>
    inline bool ObjectArrayValueProxy<T, V>::Get(const json::r::Value& document, const std::string& key,
                                                 void* obj) const
    {
        T* inst = (T*)obj;
        json::r::Value jsonArray;
        if (!document.Get(key.c_str(), jsonArray) || !jsonArray.IsArray())
        {
            if (!this->mMust)
            {
                return true;
            }
            return false;
        }
        size_t index = 0;
        json::r::Value jsonObject;
        while (jsonArray.Get(index, jsonObject))
        {
            index++;
            V value;
            if (!value.Decode(jsonObject))
            {
                return false;
            }
            (inst->*field).emplace_back(value);
        }
        return true;
    }

    template <typename T, typename V>
    inline bool ObjectMapValueProxy<T, V>::Set(json::w::Value& document, const std::string& key, void* obj)
    {
        T* inst = (T*)obj;
        std::unique_ptr<json::w::Value> jsonObject = document.AddObject(key.c_str());
        {
            auto iter = (inst->*field).begin();
            for (; iter != (inst->*field).end(); iter++)
            {
                std::unique_ptr<json::w::Value> jsonField = jsonObject->AddObject(iter->first.c_str());
                if (!iter->second.Encode(*jsonField))
                {
                    return false;
                }
            }
        }
        return true;
    }

    template <typename T, typename V>
    inline bool ObjectMapValueProxy<T, V>::Get(const json::r::Value& document, const std::string& key, void* obj) const
    {
        T* inst = (T*)obj;
        json::r::Value jsonObject;
        if (!document.Get(key.c_str(), jsonObject) || !jsonObject.IsObject())
        {
            if (!this->mMust)
            {
                return true;
            }
            return false;
        }
        std::vector<const char*> keys = jsonObject.GetAllKey();
        for (const char* fieldKey : keys)
        {
            json::r::Value jsonField;
            if (jsonObject.Get(fieldKey, jsonField))
            {
                V value;
                if (!value.Decode(jsonField))
                {
                    return false;
                }
                (inst->*field).emplace(fieldKey, value);
            }
        }
        return true;
    }

    template <typename T, typename V>
    inline bool MapValueProxy<T, V>::Get(const json::r::Value& document, const std::string& key,
                                         void* obj) const
    {
        T* inst = (T*)obj;
        json::r::Value jsonObject;
        if (!document.Get(key.c_str(), jsonObject) || !jsonObject.IsObject())
        {
            if (!this->mMust)
            {
                return true;
            }
            return false;
        }
        std::vector<const char*> keys = jsonObject.GetAllKey();
        for (const char* fieldKey : keys)
        {
            V value;
            if (jsonObject.Get(fieldKey, value))
            {
                (inst->*field).emplace(fieldKey, value);
            }
        }
        return true;
    }

    template <typename T, typename V>
    inline bool MapValueProxy<T, V>::Set(json::w::Value& document, const std::string& key, void* obj)
    {
        T* inst = (T*)obj;
        std::unique_ptr<json::w::Value> jsonObject = document.AddObject(key.c_str());
        {
            for (auto iter = (inst->*field).begin(); iter != (inst->*field).end(); iter++)
            {
                jsonObject->Add(iter->first.c_str(), iter->second);
            }
        }
        return true;
    }

    template <typename T>
    std::unordered_map<std::string, std::unique_ptr<ValueBase>> Object<T>::values;
}

#define REGISTER_JSON_CLASS(type) 	type::RegisterAllFields()
#define REGISTER_JSON_CLASS_FIELD(T, field) T::RegisterField(#field, &T::field)
#define REGISTER_JSON_CLASS_MUST_FIELD(T, field) T::RegisterField(#field, &T::field, true)

#endif //APP_JSONOBJECT_H
