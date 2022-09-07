//
// Created by mac on 2022/5/18.
//

#include"MongoClient.h"

#include<regex>
#include"Util/MD5.h"
#include"Util/sha1.h"
#include"Bson/base64.h"
#include"Util/StringHelper.h"
#include"Component/Mongo/MongoRpcComponent.h"

namespace Mongo
{
	TcpMongoClient::TcpMongoClient(std::shared_ptr<SocketProxy> socket, const Mongo::Config& config)
		: Tcp::TcpContext(socket, 1024 * 1024), mConfig(config), mMongoComponent(nullptr)
	{
        LOG_CHECK_FATAL(this->mMongoComponent = App::Get()->GetComponent<MongoRpcComponent>());
	}

	void TcpMongoClient::OnSendMessage(const asio::error_code& code, std::shared_ptr<ProtoMessage> message)
	{
		int len = 0;
		std::string json;
		std::shared_ptr<MongoQueryRequest> resuet = std::dynamic_pointer_cast<MongoQueryRequest>(message);
		Bson::Reader::Document document(resuet->document.Serialize(len));
		document.WriterToJson(json);
		CONSOLE_LOG_ERROR("send json = " << json);

		if (code)
		{
			if(!this->StartAuthBySha1())
			{
				CONSOLE_LOG_ERROR("auth mongo user failure");
				return;
			}
            this->SendFromMessageQueue();
            std::string address = fmt::format(
                    "{0}:{1}",this->mConfig.mIp, this->mConfig.mPort);
            CONSOLE_LOG_INFO( "["<< address << "] mongo user auth successful");
            return;
		}
        assert(this->mRecvBuffer.size() == 0);
        assert(this->mSendBuffer.size() == 0);
        assert(this->mMongoResponse == nullptr);
		this->ReceiveMessage(sizeof(MongoHead));
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
            std::string address = fmt::format(
                    "{0}:{1}",this->mConfig.mIp, this->mConfig.mPort);
            CONSOLE_LOG_INFO( "["<< address << "] mongo user auth successful");
            return;
		}

		if (this->mMongoResponse == nullptr)
		{
			assert(this->mRecvBuffer.size() >= sizeof(MongoHead));
			this->mMongoResponse = std::make_shared<MongoQueryResponse>();
			this->ReceiveMessage(this->mMongoResponse->OnReceiveHead(is));
		}
		else
		{
			this->mMongoResponse->OnReceiveBody(is);
#ifdef __DEBUG__
            std::shared_ptr<MongoQueryRequest> request =
                    static_pointer_cast<MongoQueryRequest>(this->PopMessage());
            assert(request->header.requestID == this->mMongoResponse->GetHead().responseTo);
#endif
			long long responseId = this->mMongoResponse->GetHead().responseTo;
			std::shared_ptr<MongoQueryResponse> response = std::move(this->mMongoResponse);
#ifdef ONLY_MAIN_THREAD
			this->mMongoComponent->OnResponse(responseId, response);
#else
			asio::io_service& io = App::Get()->GetThread();
			io.post(std::bind(&MongoRpcComponent::OnResponse,
				this->mMongoComponent, responseId, response));
#endif
			//CONSOLE_LOG_INFO("成功次数 = " << SuccessfulCount);
            this->PopMessage();
			this->SendFromMessageQueue();
		}

	}

	void TcpMongoClient::SendMongoCommand(std::shared_ptr<MongoQueryRequest> request)
	{
#ifdef ONLY_MAIN_THREAD
		this->Send(request);
#else
        asio::io_service & t = this->mSocket->GetThread();
		t.post(std::bind(&TcpMongoClient::Send, this, request));
#endif
	}

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

	std::shared_ptr<MongoQueryResponse> TcpMongoClient::SyncSendMongoCommand(std::shared_ptr<MongoQueryRequest> request)
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
		std::shared_ptr<MongoQueryResponse> response(new MongoQueryResponse());
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
		if(!this->ConnectSync())
		{
			CONSOLE_LOG_ERROR("connect mongo error");
			return false;
		}
		if(this->mConfig.mPasswd.empty())
		{
			return true;
		}
		std::string nonce = _bson::base64::encode(Helper::String::RandomString(8));
		std::shared_ptr<Mongo::MongoQueryRequest> request1(new MongoQueryRequest());
		std::string firstBare = fmt::format("n={0},r={1}", this->mConfig.mUser, nonce);

		std::string payload = _bson::base64::encode(fmt::format("n,,{0}", firstBare));

		request1->header.requestID = 1;
		request1->collectionName = this->mConfig.mDb + ".$cmd";
		request1->document.Add("saslStart", 1);
		request1->document.Add("autoAuthorize", 1);
		request1->document.Add("mechanism", "SCRAM-SHA-1");
		request1->document.Add("payload", payload);

		if(this->SendSync(request1) <= 0)
		{
			return false;
		}
		if(this->RecvSync(sizeof(MongoHead)) <= 0)
		{
			return false;
		}
        MongoQueryResponse response1;
		std::istream readStream1(&this->mRecvBuffer);
		if(this->RecvSync(response1.OnReceiveHead(readStream1)) <= 0)
		{
			return false;
		}

		int conversationId = 0;
		std::string server_first;
        if(response1.OnReceiveBody(readStream1) <= 0)
        {
            return false;
        }
		if(!response1[0].Get("payload", server_first) || !response1[0].Get("conversationId", conversationId))
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
		std::string pbkdf2_key = Helper::Md5::SumHex(fmt::format(
			"{0}:mongo:{1}",this->mConfig.mUser,this->mConfig.mPasswd));
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


		std::shared_ptr<Mongo::MongoQueryRequest> request2(new MongoQueryRequest());
		request2->header.requestID = 1;
		request2->collectionName = this->mConfig.mDb + ".$cmd";

		request2->document.Add("saslContinue", 1);
		request2->document.Add("conversationId", conversationId);
		request2->document.Add("payload", client_final);
		if(this->SendSync(request2) <= 0)
		{
			return false;
		}
		if(this->RecvSync(sizeof(MongoHead)) <= 0)
		{
			return false;
		}
        MongoQueryResponse response2;
		if(this->RecvSync(response2.OnReceiveHead(readStream1)) <= 0)
		{
			return false;
		}
        if(response2.OnReceiveBody(readStream1) <=0 )
        {
            return false;
        }
        parsedSource.clear();
		if(!response2[0].Get("payload", parsedSource))
		{
			return false;
		}
		bool done = false;
		if(response2[0].Get("done", done) && done)
		{
			return true;
		}
		std::shared_ptr<MongoQueryRequest> request3(new MongoQueryRequest());
		request3->header.requestID = 1;
		request3->collectionName = this->mConfig.mDb + ".$cmd";
		request3->document.Add("saslContinue", 1);
		request3->document.Add("conversationId", conversationId);
		request3->document.Add("payload", "");

		if(this->SendSync(request2) <= 0 || this->RecvSync(sizeof(MongoHead)) <= 0)
		{
			return false;
		}
        MongoQueryResponse response3;
		if(this->RecvSync(response3.OnReceiveHead(readStream1)) <= 0)
		{
			return false;
		}
        if(response3.OnReceiveBody(readStream1) <=0)
        {
            return false;
        }
		if(!response3[0].IsOk())
		{
			return false;
		}
		return response3[0].Get("done", done) && done;
	}

}