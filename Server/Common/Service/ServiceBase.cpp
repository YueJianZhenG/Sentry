#include"ServiceBase.h"
#include<Manager/ActionManager.h>
#include<NetWork/NetWorkRetAction.h>
#include<Coroutine/CoroutineManager.h>
namespace SoEasy
{
	ServiceBase::ServiceBase()
	{
		this->mIsInit = false;
		this->mServiceId = 0;
	}
	bool ServiceBase::OnInit()
	{
		SayNoAssertRetFalse_F(this->mActionManager = this->GetManager<ActionManager>());
		SayNoAssertRetFalse_F(this->mCorManager = this->GetManager<CoroutineManager>());
		return true;
	}

	void ServiceBase::Sleep(long long ms)
	{
		SayNoAssertRet_F(this->mCorManager->IsInMainCoroutine());
		this->mCorManager->Sleep(ms);
	}

	void ServiceBase::Start(const std::string & name, std::function<void()> && func)
	{
		SayNoAssertRet_F(this->mCorManager->IsInMainCoroutine());
		this->mCorManager->Start(name, std::move(func));
	}

	void ServiceBase::InitService(int serviceId)
	{
		if (this->mIsInit == false)
		{
			this->mIsInit = true;
			this->mServiceId = serviceId;
		}
	}

	XCode ServiceBase::Call(const std::string method, Message & returnData)
	{
		shared_ptr<NetWorkWaitCorAction> callBack = NetWorkWaitCorAction::Create(method, this->mCorManager);
		if (callBack == nullptr)
		{
			return XCode::NoCoroutineContext;
		}
		if (!this->HandleMessage(this->CreatePacket(method, nullptr, callBack)))
		{
			return XCode::HandleMessageFail;
		}
		this->mCorManager->YieldReturn();
		if (!returnData.ParseFromString(callBack->GetMsgData()))
		{
			return XCode::ParseMessageError;
		}
		return callBack->GetCode();
	}

	XCode ServiceBase::Call(const std::string method, shared_ptr<Message> message)
	{
		shared_ptr<NetWorkWaitCorAction> callBack = NetWorkWaitCorAction::Create(method, this->mCorManager);
		if (this->mCorManager->IsInMainCoroutine())
		{
			return XCode::NoCoroutineContext;
		}
		if (!this->HandleMessage(this->CreatePacket(method, nullptr, callBack)))
		{
			return XCode::HandleMessageFail;
		}
		this->mCorManager->YieldReturn();
		return callBack->GetCode();
	}

	XCode ServiceBase::Call(const std::string method, shared_ptr<Message> message, Message & returnData)
	{
		shared_ptr<NetWorkWaitCorAction> callBack = NetWorkWaitCorAction::Create(method, this->mCorManager);
		if (callBack == nullptr)
		{
			return XCode::NoCoroutineContext;
		}
		if (!this->HandleMessage(this->CreatePacket(method, nullptr, callBack)))
		{
			return XCode::HandleMessageFail;
		}
		this->mCorManager->YieldReturn();
		if (!returnData.ParseFromString(callBack->GetMsgData()))
		{
			return XCode::ParseMessageError;
		}
		return callBack->GetCode();
	}
	shared_ptr<NetWorkPacket> ServiceBase::CreatePacket(const std::string & func, shared_ptr<Message> message, shared_ptr<NetWorkWaitCorAction> callBack)
	{
		shared_ptr<NetWorkPacket> callData = make_shared<NetWorkPacket>();
		if (message != nullptr)
		{
			std::string messageBuffer;
			if (!message->SerializePartialToString(&messageBuffer))
			{
				SayNoDebugError(func << " " << message->GetTypeName() << " Serialize fail");
				return nullptr;
			}
			callData->set_message_data(messageBuffer);
			callData->set_protoc_name(message->GetTypeName());
		}
		long long id = 0;
		this->mActionManager->AddCallback(callBack, id);
		callData->set_action(func);
		callData->set_callback_id(id);
		callData->set_service(this->GetTypeName());
		return callData;
	}
}
