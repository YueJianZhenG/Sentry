//
// Created by mac on 2022/5/18.
//

#include"MongoClient.h"

#include<regex>
#include"Util/MD5.h"
#include"Util/sha1.h"
#include"Bson/base64.h"
#include"Util/StringHelper.h"
#include"Component/Mongo/MongoComponent.h"

namespace Mongo
{
	MongoClientContext::MongoClientContext(std::shared_ptr<SocketProxy> scoket,
			const Mongo::Config& config, MongoComponent* component, int index)
		: Tcp::TcpContext(scoket), mConfig(config), mMongoComponent(component), mIndex(index)
	{

	}

	void MongoClientContext::OnSendMessage(const asio::error_code& code, std::shared_ptr<ProtoMessage> message)
	{
		if (code)
		{
#ifdef __DEBUG__
			CONSOLE_LOG_ERROR(code.message());
#endif
			this->ClearSendStream();
			if(!this->StartAuthBySha1())
			{
				CONSOLE_LOG_ERROR("auth mongo user failure");
				return;
			}
		}

		this->mCommands.pop_front();
		this->mReadState == ReadType::HEAD;
		this->ReceiveMessage(sizeof(MongoHead));
	}

    void MongoClientContext::OnReceiveMessage(const asio::error_code &code, asio::streambuf &buffer)
	{
		if (code)
		{
#ifdef ONLY_MAIN_THREAD
			this->mMongoComponent->OnClientError(this->mIndex, XCode::NetReceiveFailure);
#else
			MainTaskScheduler& taskScheduler = App::Get()->GetTaskScheduler();
			taskScheduler
				.Invoke(&MongoComponent::OnClientError, this->mMongoComponent, this->mIndex, XCode::NetReceiveFailure);
#endif
			CONSOLE_LOG_ERROR(code.message());
			return;
		}
		std::iostream os(&buffer);
		if (this->mReadState == ReadType::HEAD)
		{
			this->mReadState = ReadType::BODY;
			int length = mMongoResponse.OnReceiveHead(os);
			this->ReceiveMessage(length - sizeof(MongoHead));
			return;
		}
		const MongoHead& mongoHead = this->mMongoResponse.GetHead();
		std::shared_ptr<Bson::ReaderDocument> res = this->mMongoResponse.OnReceiveBody(os);
		std::string json;
		res->WriterToJson(json);

#ifdef ONLY_MAIN_THREAD
		this->mMongoComponent->OnResponse(mongoHead.responseTo, res);
#else
		MainTaskScheduler& taskScheduler = App::Get()->GetTaskScheduler();
		taskScheduler.Invoke(&MongoComponent::OnResponse, this->mMongoComponent, mongoHead.responseTo, res);
#endif

		if (!this->mCommands.empty())
		{
			this->Send(this->mCommands.front());
		}
		this->mReadState = ReadType::HEAD;
	}

	void MongoClientContext::PushMongoCommand(std::shared_ptr<Tcp::ProtoMessage> request)
	{
#ifdef ONLY_MAIN_THREAD
		this->PushCommand(request, front);
#else
		this->mNetworkThread.Invoke(&MongoClientContext::PushCommand, this, request);
#endif
	}

	void MongoClientContext::PushCommand(std::shared_ptr<Tcp::ProtoMessage> request)
	{
		if(this->mCommands.empty())
		{
			this->Send(request);
		}
		this->mCommands.push_back(request);
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

	bool MongoClientContext::StartAuthUser()
	{
		if(!this->ConnectSync())
		{
			CONSOLE_LOG_ERROR("connect mongo error");
			return false;
		}

		std::string password = Helper::Md5::SumHex(fmt::format(
			"{0}:mongo:{1}", this->mConfig.mUser, this->mConfig.mPasswd));
		std::shared_ptr<Mongo::MongoQueryRequest> request1(new MongoQueryRequest());

		CONSOLE_LOG_ERROR(password);

		request1->header.requestID = 1;
		request1->collectionName = this->mConfig.mDb + ".$cmd";
		request1->document.Add("getnonce", 1);
		if(this->SendSync(request1) <= 0 || this->RecvSync(sizeof(MongoHead)) <= 0)
		{
			return false;
		}
		std::istream & readStream1 = this->GetReadStream();
		int length = this->mMongoResponse.OnReceiveHead(readStream1);
		if(this->RecvSync(length - sizeof(MongoHead)) <= 0)
		{
			return false;
		}
		std::string nonce;
		std::shared_ptr<Bson::ReaderDocument> response1 = this->mMongoResponse.OnReceiveBody(readStream1);
		if(response1 == nullptr || !response1->Get("nonce", nonce))
		{
			return false;
		}
		std::string key = Helper::Md5::SumHex(nonce + this->mConfig.mUser + password);

		CONSOLE_LOG_ERROR(key);
		std::shared_ptr<Mongo::MongoQueryRequest> request2(new MongoQueryRequest());

		request2->header.requestID = 2;
		request2->collectionName = this->mConfig.mDb + ".$cmd";
		request2->document.Add("authenticate", 1);
		request2->document.Add("user", this->mConfig.mUser);
		request2->document.Add("nonce", nonce);
		request2->document.Add("key", key);
		if(this->SendSync(request2) <= 0 || this->RecvSync(sizeof(MongoHead)) <= 0)
		{
			return false;
		}
		length = this->mMongoResponse.OnReceiveHead(readStream1);
		if(this->RecvSync(length - sizeof(MongoHead)) <= 0)
		{
			return false;
		}
		std::string json;
		std::shared_ptr<Bson::ReaderDocument> response2 = this->mMongoResponse.OnReceiveBody(readStream1);
		response2->WriterToJson(json);
		return true;
	}

	bool MongoClientContext::StartAuthBySha1()
	{
		if(!this->ConnectSync())
		{
			CONSOLE_LOG_ERROR("connect mongo error");
			return false;
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

		std::istream & readStream1 = this->GetReadStream();
		int length = this->mMongoResponse.OnReceiveHead(readStream1);
		if(this->RecvSync(length - sizeof(MongoHead)) <= 0)
		{
			return false;
		}

		int conversationId = 0;
		std::string server_first;
		std::shared_ptr<Bson::ReaderDocument> response1 = this->mMongoResponse.OnReceiveBody(readStream1);
		if(response1 == nullptr || !response1->Get("payload", server_first) || !response1->Get("conversationId", conversationId))
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

		length = this->mMongoResponse.OnReceiveHead(readStream1);
		if(this->RecvSync(length - sizeof(MongoHead)) <= 0)
		{
			return false;
		}
		std::shared_ptr<Bson::ReaderDocument> response2 = this->mMongoResponse.OnReceiveBody(readStream1);
		if(response1 == nullptr || length <= sizeof(MongoHead) || response2 == nullptr)
		{
			return false;
		}
		std::string json2;
		response2->WriterToJson(json2);
		return true;
	}

}