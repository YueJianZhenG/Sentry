//
// Created by zmhy0073 on 2021/10/11.
//

#include "GatewayService.h"
#include <Object/GameObject.h>
#include <Scene/RedisComponent.h>
#include <Scene/GatewayComponent.h>
#include <Scene/GameObjectComponent.h>
#include <Scene/NodeMaperComponent.h>
namespace GameKeeper
{
    bool GatewayService::Awake()
    {
        __add_method(GatewayService::Login);
        __add_method(GatewayService::Logout);
        this->mRedisComponent = this->GetComponent<RedisComponent>();
        this->mGateComponent = this->GetComponent<GatewayComponent>();
        this->mGameObjComponent = this->GetComponent<GameObjectComponent>();
        return true;
    }

    XCode GatewayService::Login(long long id, c2s::GateLogin_Request & request)
    {
        std::string queryData;
        const std::string & token = request.token();
        long long socketId  = this->GetCurSocketId();
        if(!this->mRedisComponent->GetValue(token,queryData))
        {
            return XCode::Failure;
        }
        long long userId = std::stoll(queryData);
        auto gameObject = new GameObject(userId, socketId);
        if(!this->mGameObjComponent->Add(gameObject))
        {
            return XCode::Failure;
        }
        // 给连接到网关的对象加组件
        gameObject->AddComponent<NodeMaperComponent>();

        return XCode::Successful;
    }

    XCode GatewayService::Logout(long long id, c2s::GateLogout_Request & request)
    {
        this->mGameObjComponent->Del(gameObject);
        return XCode::Successful;
    }
}