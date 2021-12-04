﻿#include "AccountService.h"
#include <Core/App.h>
#include <Util/MD5.h>
#include <Util/MathHelper.h>
#include <Scene/RedisComponent.h>
#include <Scene/MysqlProxyComponent.h>
#include"MysqlClient/MysqlRpcTask.h"
#include<google/protobuf/util/json_util.h>
namespace GameKeeper
{
    AccountService::AccountService()
    {
        this->mRedisComponent = nullptr;
        this->mMysqlComponent = nullptr;
    }

    bool AccountService::Awake()
    {
        BIND_RPC_FUNCTION(AccountService::Login);
        BIND_RPC_FUNCTION(AccountService::Register);
        this->mRedisComponent = this->GetComponent<RedisComponent>();
        this->mMysqlComponent = this->GetComponent<MysqlProxyComponent>();
		return true;
    }

    void AccountService::Start()
    {

    }

    XCode AccountService::Login(const c2s::AccountLogin_Request& request, c2s::AccountLogin_Response & response)
    {

        return XCode::Successful;
    }

    XCode AccountService::Register(const c2s::AccountRegister_Request & request, c2s::AccountRegister_Response & response)
    {
        const std::string &account = request.account();
        const std::string &password = request.passwd();
        if (account.empty() || password.empty())
        {
            return XCode::CallArgsError;
        }

        auto queryResponse = this->mRedisComponent->Invoke("HEXISTS", "user", account);
        if (queryResponse->GetNumber() == 1)
        {
            return XCode::AccountAlreadyExists;
        }
        long long userId = USER_ID_START + this->mRedisComponent->AddCounter("userid");

        db::UserAccountData userAccountData;

        std::string token = this->NewToken(account);

        userAccountData.set_token(token);
        userAccountData.set_userid(userId);
        userAccountData.set_account(account);
        userAccountData.set_passwd(password);
        userAccountData.set_registertime(TimeHelper::GetSecTimeStamp());
#ifdef __DEBUG__
        std::string json;
        util::MessageToJsonString(userAccountData, &json);
        LOG_DEBUG("register new player json = " << json);
#endif
        const int second = 7 * 24 * 60 * 60;
        response.set_token(userAccountData.token());
        this->mRedisComponent->Invoke("SETEX", token, second, account);
        this->mRedisComponent->Invoke("HSET", "user", account, userAccountData);
        XCode code = this->mMysqlComponent->Add(userAccountData)->AwakeGetCode();
        if(code != XCode::Successful)
        {
            LOG_ERROR(account << " register failure");
            return code;
        }
        LOG_INFO(account << " register successful");
        return XCode::Successful;
    }

    const std::string AccountService::NewToken(const std::string & account)
    {
        char buffer[100] = {0};
        int number = MathHelper::Random<int>();
        long long now = TimeHelper::GetSecTimeStamp();
        size_t size = sprintf(buffer, "%s:%lld:%d", account.c_str(), now, number);
        MD5 md5(buffer, size);
        return md5.toString();
    }
}// namespace GameKeeper