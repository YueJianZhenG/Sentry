﻿#pragma once

#include<memory>
#include<Method/SubMethod.h>
#include<Component/Component.h>
#include<Method/ServiceMethod.h>

using namespace std;
using namespace com;

namespace Sentry
{
    class ServiceMethod;
    class RcpService : public Component {
    public:
        RcpService() = default;

        ~RcpService() override = default;

    public:

        virtual const std::string & GetName() { return this->GetTypeName();}

        std::shared_ptr<com::Rpc_Response> Invoke(const std::string &method, std::shared_ptr<com::Rpc_Request> request);

    public:
        bool AddMethod(ServiceMethod *method);

        long long GetCurSocketId() const { return this->mCurSocketId; }

        void SetCurSocketId(long long socketId) { this->mCurSocketId = socketId; }

    protected:
        template<typename T>
        bool Bind(std::string name, ServiceMethodType1<T> func)
        {
            return this->AddMethod(new ServiceMethod1<T>(name, (T *) this, func));
        }

        template<typename T>
        bool Bind(std::string name, ServiceMethodType11<T> func)
        {
            return this->AddMethod(new ServiceMethod1<T>(name, (T *) this, func));
        }

        template<typename T, typename T1>
        bool Bind(std::string name, ServiceMethodType2<T, T1> func)
        {
            return this->AddMethod(new ServiceMethod2<T, T1>(name, (T *) this, func));
        }

        template<typename T, typename T1>
        bool Bind(std::string name, ServiceMethodType22<T, T1> func)
        {
            return this->AddMethod(new ServiceMethod2<T, T1>(name, (T *) this, func));
        }

        template<typename T, typename T1, typename T2>
        bool Bind(std::string name, ServiceMethodType3<T, T1, T2> func)
        {
            return this->AddMethod(new ServiceMethod3<T, T1, T2>(name, (T *) this, func));
        }

        template<typename T, typename T1, typename T2>
        bool Bind(std::string name, ServiceMethodType33<T, T1, T2> func)
        {
            return this->AddMethod(new ServiceMethod3<T, T1, T2>(name, (T *) this, func));
        }

        template<typename T, typename T1>
        bool Bind(std::string name, ServiceMethodType4<T, T1> func)
        {
            return this->AddMethod(new ServiceMethod4<T, T1>(name, (T *) this, func));
        }

        template<typename T, typename T1>
        bool Bind(std::string name, ServiceMethodType44<T, T1> func)
        {
            return this->AddMethod(new ServiceMethod4<T, T1>(name, (T *) this, func));
        }

    private:
        ServiceMethod * GetMethod(const std::string & name);

    private:
        long long mCurSocketId;
        std::unordered_map<std::string, ServiceMethod *> mMethodMap;
        std::unordered_map<std::string, ServiceMethod *> mLuaMethodMap;
    };
#define BIND_RPC_FUNCTION(func) LOG_CHECK_RET_FALSE(this->Bind(GetFunctionName(#func), &func))

}