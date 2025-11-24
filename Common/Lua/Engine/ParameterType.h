#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
// 普通数据类型
struct lua_State;
namespace Lua
{
    class ILuaWriter
    {
    public:
        virtual ~ILuaWriter() = default;
        virtual void Writer(lua_State *lua) = 0;
    };

    namespace CommonParameter
    {
        template<typename T>
        struct IsCommonParameter
        {
            constexpr static bool value = std::is_integral<T>::value ||
					std::is_floating_point<T>::value || std::is_enum<T>::value || std::is_void<T>::value;
        };

        template<> struct IsCommonParameter<bool>
        {
            constexpr static bool value = true;
        };

        template<> struct IsCommonParameter<std::string>
        {
            constexpr static bool value = true;
        };

        template<> struct IsCommonParameter<const std::string>
        {
            constexpr static bool value = true;
        };

        template<> struct IsCommonParameter<const std::string &>
        {
            constexpr static bool value = true;
        };

        template<> struct IsCommonParameter<std::string &>
        {
            constexpr static bool value = true;
        };

        template<> struct IsCommonParameter<const char *>
        {
            constexpr static bool value = true;
        };

        template<> struct IsCommonParameter<char *>
        {
            constexpr static bool value = true;
        };

    }// namespace CommonParameter
// std容器类型
    namespace ContainerParameter
    {
        template<typename T>
        struct IsContainerParameter
        {
            constexpr static bool value = false;
        };

        template<typename T>
        struct IsContainerParameter<const T>
        {
            constexpr static bool value = false;
        };

        template<typename T>
        struct IsContainerParameter<std::vector<T>>
        {
            constexpr static bool value = true;
        };

        template<typename T>
        struct IsContainerParameter<std::vector<T> &>
        {
            constexpr static bool value = true;
        };

        template<typename T>
        struct IsContainerParameter<std::vector<T> *>
        {
            constexpr static bool value = true;
        };

        template<typename T>
        struct IsContainerParameter<std::shared_ptr<std::vector<T>>>
        {
            constexpr static bool value = true;
        };

        template<typename Key, typename Value>
        struct IsContainerParameter<std::map<Key, Value>>
        {
            constexpr static bool value = true;
        };

        template<typename Key, typename Value>
        struct IsContainerParameter<std::map<Key, Value> &>
        {
            constexpr static bool value = true;
        };

        template<typename Key, typename Value>
        struct IsContainerParameter<std::map<Key, Value> *>
        {
            constexpr static bool value = true;
        };

        template<typename Key, typename Value>
        struct IsContainerParameter<std::unordered_map<Key, Value>>
        {
            constexpr static bool value = true;
        };

        template<typename Key, typename Value>
        struct IsContainerParameter<std::unordered_map<Key, Value> &>
        {
            constexpr static bool value = true;
        };

        template<typename Key, typename Value>
        struct IsContainerParameter<std::unordered_map<Key, Value> *>
        {
            constexpr static bool value = true;
        };
    }// namespace ContainerParameter
// 函数类型
    namespace FunctionParameter
    {
        template<typename T>
        struct IsFunctionParameter
        {
            constexpr static bool value = false;
        };

        template<typename Ret, typename... Args>
        struct IsFunctionParameter<std::function<Ret(Args...)>>
        {
            constexpr static bool value = true;
        };

    }// namespace FunctionParameter

    namespace TableParameter
    {

    }

    namespace ConstParameter
    {
        template<typename T>
        struct ConstProxy
        {
            using Type = T;
            constexpr static bool value = false;
        };

        template<typename T>
        struct ConstProxy<const T>
        {
            using Type = T;
            constexpr static bool value = true;
        };

        template<typename T>
        struct IsConstParameter
        {
            constexpr static bool value = false;
        };

        template<typename T>
        struct IsConstParameter<const T>
        {
            constexpr static bool value = true;
        };

        template<typename T>
        struct IsConstParameter<const T *>
        {
            constexpr static bool value = true;
        };

        template<typename T>
        struct IsConstParameter<const T &>
        {
            constexpr static bool value = true;
        };
    }// namespace ConstParameter


// 自定义数据类型
    namespace UserDataParameter
    {
        template<typename T>
        struct IsUserDataParameter
        {
            static constexpr bool value =
                    !FunctionParameter::IsFunctionParameter<T>::value &&
                    !ContainerParameter::IsContainerParameter<T>::value &&
                    !CommonParameter::IsCommonParameter<T>::value &&
                    !ConstParameter::IsConstParameter<T>::value &&
                    !std::is_enum<T>::value;
        };
    }// namespace UserDataParameter
}