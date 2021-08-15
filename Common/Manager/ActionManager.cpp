﻿#include"ActionManager.h"
#include"ScriptManager.h"
#include"NetSessionManager.h"
#include<Util/StringHelper.h>
#include<Core/Applocation.h>
#include<Util/NumberHelper.h>
#include<Timer/TimerManager.h>
#include<NetWork/NetWorkRetAction.h>
#include<Coroutine/CoroutineManager.h>
#include<Timer/ActionTimeoutTimer.h>

namespace Sentry
{
    ActionManager::ActionManager()
    {
        this->mMessageTimeout = 0;
    }

    bool ActionManager::OnInit()
    {
        this->GetConfig().GetValue("MsgTimeout", this->mMessageTimeout);
        SayNoAssertRetFalse_F(this->mTimerManager = this->GetManager<TimerManager>());
        return true;
    }

    long long ActionManager::AddCallback(shared_ptr<LocalRetActionProxy> rpcAction)
    {
        if (rpcAction == nullptr)
        {
            return 0;
        }
        long long callbackId = NumberHelper::Create();
        this->mRetActionMap.emplace(callbackId, rpcAction);
        if (this->mMessageTimeout != 0)// 添加超时
        {
            shared_ptr<ActionTimeoutTimer> timer = make_shared<ActionTimeoutTimer>(this->mMessageTimeout, callbackId,
                                                                                   this);
            this->mTimerManager->AddTimer(timer);
        }
        this->mRetActionMap.emplace(callbackId, rpcAction);
        return callbackId;
    }

    bool ActionManager::InvokeCallback(NetMessageProxy *messageData)
    {
        long long rpcId = messageData->GetRpcId();
        auto iter = this->mRetActionMap.find(rpcId);
        if (iter != this->mRetActionMap.end())
        {
            shared_ptr<LocalRetActionProxy> action = iter->second;
            if (action != nullptr)
            {
                action->Invoke(messageData);
#ifdef SOEASY_DEBUG
                const ProtocolConfig *config = messageData->GetProConfig();
                double t = (TimeHelper::GetMilTimestamp() - action->GetCreateTime()) / 1000.0f;
                SayNoDebugWarning(
                        "call " << config->ServiceName << "." << config->MethodName << " response" << " [" << t
                                << "s]");
#endif
            }
            this->mRetActionMap.erase(iter);
            return true;
        }
        return false;
    }
}