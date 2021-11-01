#include "TelnetClientComponent.h"

namespace GameKeeper
{
	TelnetClientComponent::TelnetClientComponent()
	{

	}

	TelnetClientComponent::~TelnetClientComponent()
	{

	}

	bool TelnetClientComponent::Awake()
	{
		return true;
	}

	GameKeeper::SessionBase * TelnetClientComponent::CreateSocket()
	{
		return new TelnetClientSession(this);
	}

	void TelnetClientComponent::OnCloseSession(TelnetClientSession * session)
	{
        delete session;
	}

	void TelnetClientComponent::OnListenNewSession(TelnetClientSession *session, const asio::error_code &err)
	{
		std::string * message = new std::string("welcome GameKeeper server\n enter user and password:\n>>");
		session->SendNetMessage(message);
	}

	bool TelnetClientComponent::OnReceiveMessage(TelnetClientSession * session, const std::string & message)
    {
        if (message == "exit")
        {
            return false;
        }
        std::string *newMessage = this->mStringPool.New("你说");
        newMessage->append(message + "\n");
        session->SendNetMessage(newMessage);
        return true;
    }

	void TelnetClientComponent::OnSessionError(TelnetClientSession * session, const asio::error_code & err)
	{
	}

	void TelnetClientComponent::OnConnectRemoteAfter(TelnetClientSession * session, const asio::error_code & err)
	{

	}

}