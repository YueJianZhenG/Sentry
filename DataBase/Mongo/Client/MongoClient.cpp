//
// Created by mac on 2022/5/18.
//

#include"MongoClient.h"

#include"Md5/MD5.h"
#include"Sha1/sha1.h"
#include"Bson/base64.h"
#include"String/StringHelper.h"
#include"Config/MongoConfig.h"


namespace Mongo
{
    std::string SaltPassword(std::string & pwd, std::string salt, int iter)
    {
        salt = salt + '\0' + '\0' + '\0' + '\1';
        std::string output = Helper::Sha1::GetHMacHash(pwd, salt);
        std::string inter(output);
        for(int index = 2; index <= iter; index++)
        {
            inter = Helper::Sha1::GetHMacHash(pwd, inter);
            output = Helper::Sha1::XorString(output, inter);
        }
        return output;
    }

    void TcpMongoClient::Stop()
    {
        //TODO
    }

    TcpMongoClient::TcpMongoClient(std::shared_ptr<SocketProxy> socket, IRpc<CommandResponse>* component)
		: Tcp::TcpContext(socket, 1024 * 1024), mComponent(component)
	{
        this->mIndex = 0;
        this->mAddress = MongoConfig::Inst()->Address[0].FullAddress;
	}

	void TcpMongoClient::OnSendMessage(const Asio::Code & code, std::shared_ptr<ProtoMessage> message)
	{
		if (code)
		{
			if(!this->StartAuthBySha1())
			{
				CONSOLE_LOG_ERROR("auth mongo user failure");
				return;
			}
            this->SendFromMessageQueue();
            CONSOLE_LOG_INFO( "["<< this->mAddress << "] mongo user auth successful");
            return;
		}
        assert(this->mRecvBuffer.size() == 0);
        assert(this->mSendBuffer.size() == 0);
        assert(this->mMongoResponse == nullptr);
		this->ReceiveMessage(sizeof(MongoHead));
	}

    bool TcpMongoClient::Auth(const std::string &user, const std::string &db, const std::string &pwd)
    {
        if(pwd.empty())
        {
            return true;
        }
        std::string nonce = _bson::base64::encode(Helper::String::RandomString(8));
        std::shared_ptr<Mongo::CommandRequest> request1(new CommandRequest());
        std::string firstBare = fmt::format("n={0},r={1}", MongoConfig::Inst()->User, nonce);

        std::string payload = _bson::base64::encode(fmt::format("n,,{0}", firstBare));

        request1->header.requestID = 1;
        request1->collectionName = db + ".$cmd";
        request1->document.Add("saslStart", 1);
        request1->document.Add("autoAuthorize", 1);
        request1->document.Add("mechanism", "SCRAM-SHA-1");
        request1->document.Add("payload", payload);

        std::shared_ptr<CommandResponse> response1 =
                this->SyncSendMongoCommand(request1);
        if(response1 == nullptr && response1->GetDocumentSize() == 0)
        {
            return false;
        }
        int conversationId = 0;
        std::string server_first;
        Bson::Reader::Document & document1 = response1->Get();
        if(!document1.Get("payload", server_first) ||
           !document1.Get("conversationId", conversationId))
        {
            return false;
        }

        std::string parsedSource = _bson::base64::decode(server_first);

        std::vector<std::string> ret;
        Helper::String::Split(parsedSource, ",", ret);

        std::string salt(ret[1].c_str() + 2, ret[1].size() - 2);
        std::string rnonce(ret[0].c_str() + 2, ret[0].size() - 2);
        int iterations = std::stoi(std::string(ret[2].c_str() + 2, ret[2].size() - 2));

        std::string without_proof = "c=biws,r=" + rnonce;
        std::string pbkdf2_key = Helper::Md5::SumHex(
                fmt::format("{0}:mongo:{1}", user, pwd));
        std::string salted_pass = SaltPassword(pbkdf2_key,
                                               _bson::base64::decode(salt), iterations);

        std::string client_key = Helper::Sha1::GetHMacHash(salted_pass, "Client Key");
        std::string stored_key = Helper::Sha1::GetHash(client_key);

        std::string auth_msg = firstBare + ',' + parsedSource + ',' + without_proof;
        std::string client_sin = Helper::Sha1::GetHMacHash(stored_key, auth_msg);
        std::string client_key_xor_sig = Helper::Sha1::XorString(client_key, client_sin);
        std::string client_proof = std::string("p=") + _bson::base64::encode(client_key_xor_sig);
        std::string client_final = _bson::base64::encode(without_proof + "," + client_proof);
        std::string server_key = Helper::Sha1::GetHMacHash(salted_pass, "Server Key");
        std::string server_sig = _bson::base64::encode(Helper::Sha1::GetHMacHash(server_key, auth_msg));


        std::shared_ptr<Mongo::CommandRequest> request2(new CommandRequest());
        request2->header.requestID = 1;
        request2->collectionName = db + ".$cmd";

        request2->document.Add("saslContinue", 1);
        request2->document.Add("conversationId", conversationId);
        request2->document.Add("payload", client_final);

        std::shared_ptr<CommandResponse> response2 =
                this->SyncSendMongoCommand(request2);
        if(response2 == nullptr || response2->GetDocumentSize() == 0)
        {
            return false;
        }
        parsedSource.clear();
        Bson::Reader::Document & document2 = response2->Get();
        if(!document2.Get("payload", parsedSource))
        {
            return false;
        }
        bool done = false;
        if(document2.Get("done", done) && done)
        {
            return true;
        }
        std::shared_ptr<CommandRequest> request3(new CommandRequest());
        request3->header.requestID = 1;
        request3->collectionName = db + ".$cmd";
        request3->document.Add("saslContinue", 1);
        request3->document.Add("conversationId", conversationId);
        request3->document.Add("payload", "");

        std::shared_ptr<CommandResponse> response3 =
                this->SyncSendMongoCommand(request3);
        if(response3 == nullptr && response3->GetDocumentSize() == 0)
        {
            return false;
        }
        Bson::Reader::Document & document3 = response3->Get();
        if(document3.IsOk() && document3.Get("done", done) && done)
		{
			const std::string & address = this->mSocket->GetAddress();
#ifdef ONLY_MAIN_THREAD
			this->mComponent->OnConnectSuccessful(address);
#else
			asio::io_service& io = App::Inst()->MainThread();
			io.post(std::bind(&IRpc<CommandResponse>::OnConnectSuccessful, this->mComponent, address));
#endif
			return true;
		}
		return false;
    }

    void TcpMongoClient::OnReceiveMessage(const asio::error_code &code, std::istream & is, size_t size)
	{
		if (code)
		{
            this->mSocket->Close();
			CONSOLE_LOG_ERROR(code.message());
            if(!this->StartAuthBySha1())
            {
                CONSOLE_LOG_ERROR("auth mongo user failure");
                return;
            }
            this->SendFromMessageQueue();
            CONSOLE_LOG_INFO( "["<< this->mAddress << "] mongo user auth successful");
            return;
		}

		if (this->mMongoResponse == nullptr)
		{
			assert(this->mRecvBuffer.size() >= sizeof(MongoHead));
			this->mMongoResponse = std::make_shared<CommandResponse>();

            int len = this->mMongoResponse->OnReceiveHead(is);
			this->ReceiveMessage(len);
		}
		else
		{
			this->mMongoResponse->OnReceiveBody(is);
			const std::string & address = this->mSocket->GetAddress();
			std::shared_ptr<CommandResponse> response = std::move(this->mMongoResponse);
#ifdef ONLY_MAIN_THREAD
			this->mComponent->OnMessage(address, response);
#else
			asio::io_service& io = App::Inst()->MainThread();
			io.post(std::bind(&IRpc<CommandResponse>::OnMessage, this->mComponent, address, response));
#endif
			this->PopMessage();
			this->mMongoResponse = nullptr;
			this->SendFromMessageQueue();
		}

	}

	void TcpMongoClient::SendMongoCommand(std::shared_ptr<CommandRequest> request)
	{
#ifdef ONLY_MAIN_THREAD
		this->Write(request);
#else
        asio::io_service & t = this->mSocket->GetThread();
		t.post(std::bind(&TcpMongoClient::Write, this, request));
#endif
	}

	std::shared_ptr<CommandResponse> TcpMongoClient::SyncSendMongoCommand(std::shared_ptr<CommandRequest> request)
	{
		if(this->SendSync(request) <= 0)
		{
			return nullptr;
		}
		if(this->RecvSync(sizeof(MongoHead)) <= 0)
		{
			return nullptr;
		}
		std::istream readStream1(&this->mRecvBuffer);
		std::shared_ptr<CommandResponse> response(new CommandResponse());
		if(this->RecvSync(response->OnReceiveHead(readStream1)) <= 0)
		{
			return nullptr;
		}
		if(response->OnReceiveBody(readStream1) <= 0)
		{
			return nullptr;
		}
		return response;
	}

	bool TcpMongoClient::StartAuthBySha1()
	{
        const MongoConfig * config = MongoConfig::Inst();
        if(this->mIndex >= config->Address.size())
        {
            return false;
        }
        const Net::Address & address
            = config->Address[this->mIndex];
        this->mSocket->Init(address.Ip, address.Port);
		if(!this->ConnectSync())
		{
            this->mIndex++;
			return this->StartAuthBySha1();
		}
        this->mIndex = 0;
        return this->Auth(config->User, "admin", config->Password);
	}

}