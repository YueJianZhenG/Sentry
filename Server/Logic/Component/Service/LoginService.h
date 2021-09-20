﻿#pragma once

#include <Protocol/c2s.pb.h>
#include <Service/LocalServiceComponent.h>

namespace Sentry
{
    class MysqlComponent;

    class RedisComponent;

    class LoginService : public LocalServiceComponent
    {
    public:
        LoginService();

        ~LoginService() {}

    protected:
        bool Awake() override;

        void Start() final;

    private:
        XCode Login(long long operId, shared_ptr<c2s::UserVerify_Request> LoginData);

        XCode Register(long long operId, shared_ptr<c2s::UserRegister_Request> registerData);

    private:
        RedisComponent *mRedisManager;
        MysqlComponent *mMysqlManager;
    };
}// namespace Sentry