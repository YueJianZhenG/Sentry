#include"Command.h"
#include<Core/Applocation.h>
#include<Util/StringHelper.h>
#include<Service/ServiceBase.h>
#include<Other/ObjectFactory.h>
#include<Util/ProtocHelper.h>
#include<google/protobuf/util/json_util.h>
namespace SoEasy
{
	CommandBase::CommandBase(CommandManager * commandMgr)
	{
		this->mCommandManager = commandMgr;
	}

	void StateCommand::Invoke(SharedTelnetSession session, const std::string paramate)
	{

	}

	void ServiceListCommand::Invoke(SharedTelnetSession session, const std::string paramate)
	{
		std::stringstream returnMessage;
		std::vector<ServiceBase *> services;
		Applocation::Get()->GetServices(services);
		for (ServiceBase * service : services)
		{
			returnMessage << service->GetTypeName() << '\n';
		}
		const std::string & address = session->GetAddress();
		this->mCommandManager->AddCommandBackArgv(address, XCode::Successful, returnMessage.str());
	}
	void ServiceCallCommand::Invoke(SharedTelnetSession session, const std::string paramate)
	{
		std::vector<std::string> paramates;
		StringHelper::SplitString(paramate, ",", paramates);
		const std::string & address = session->GetAddress();
		if (paramate.size() != 4)
		{
			std::string returnData = "cmd argv error";			
			this->mCommandManager->AddCommandBackArgv(address, XCode::Failure, returnData);
			return;
		}
		const std::string & service = paramates[0];
		const std::string & action = paramates[1];
		const std::string & protoc = paramates[2];
		const std::string & json = paramates[3];
		ServiceBase * serviceProxy = Applocation::Get()->GetService(service);
		if (serviceProxy == nullptr)
		{
			std::string returnData = "not find service : " + service;
			this->mCommandManager->AddCommandBackArgv(address, XCode::Failure, returnData);
			return;
		}
		Message * message = ObjectFactory::Get()->CreateMessage(protoc);
		if (message == nullptr)
		{
			std::string returnData = "not find ptotoc : " + protoc;
			this->mCommandManager->AddCommandBackArgv(address, XCode::Failure, returnData);
			return;
		}
		if (!util::JsonStringToMessage(json, message).ok())
		{
			std::string returnData = "parse argv error : " + json;
			this->mCommandManager->AddCommandBackArgv(address, XCode::Failure, returnData);
		}
		SharedPacket messageData = make_shared<PB::NetWorkPacket>();
		messageData->set_service(paramates[0]);
		messageData->set_action(paramates[1]);
		messageData->set_protoc_name(paramates[2]);
		messageData->set_message_data(message->SerializeAsString());
		serviceProxy->AddActionArgv(messageData);
	}
}
