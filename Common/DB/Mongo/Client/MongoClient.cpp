//
// Created by mac on 2022/5/18.
//

#include <regex>
#include "MongoClient.h"
#include "XCode/XCode.h"
#include "Util/Crypt/sha1.h"
#include "Util/Crypt/MD5Helper.h"
#include "Proto/Bson/base64.h"
#include "Util/Tools/String.h"
#include "Mongo/Config/MongoConfig.h"
#include "Core/Thread/ThreadSync.h"
#include "Mongo/Client/MongoFactory.h"

#ifdef __ENABLE_OPEN_SSL__

#include <openssl/sha.h>

#endif

namespace mongo
{

	inline std::string SumHex(const std::string& key)
	{
		std::string result;
		std::regex regex1(".");
		std::string target = help::md5::GetMd5(key);
		auto begin = std::sregex_iterator(target.begin(), target.end(), regex1);
		for (auto iter = begin; iter != std::sregex_iterator(); iter++)
		{
			char temp[10] = { 0 };
			unsigned char cc = iter->str()[0];
#ifndef _MSC_VER
			result.append(temp, sprintf(temp, "%02x", (int)cc));
#else
			result.append(temp, sprintf_s(temp, "%02x", (int)cc));
#endif
		}
		return result;
	}

	inline std::string SaltPasswordBySha1(const std::string& pwd, std::string salt, int iterations)
	{
		salt = salt + '\0' + '\0' + '\0' + '\1';

		std::string output = help::Sha1::GetHMacHash(pwd, salt);
		std::string inter(output);
		for (int index = 2; index <= iterations; index++)
		{
			inter = help::Sha1::GetHMacHash(pwd, inter);
			output = help::Sha1::XorString(output, inter);
		}
		return output;
	}

#ifdef __ENABLE_OPEN_SSL__

	inline std::string SaltPasswordBySha256(const std::string& pwd, const std::string& salt, int iterations)
	{
		std::string salted(SHA256_DIGEST_LENGTH, '\0');
		PKCS5_PBKDF2_HMAC(pwd.c_str(), pwd.size(),
				(const unsigned char*)salt.c_str(), salt.size(),
				iterations, EVP_sha256(), SHA256_DIGEST_LENGTH,
				(unsigned char*)salted.data());

		return salted;
	}

#endif

	void Client::Stop()
	{
		//TODO
	}

	bool Client::Start(tcp::Socket * socket)
	{
		if(socket == nullptr)
		{
			Asio::Context& context = this->mSocket->GetContext();
			asio::post(context, [this, self = this->shared_from_this()]() {
				this->mConnectCount = 0;
				this->Connect(5);
			});
			return true;
		}
		this->SetSocket(socket);
#ifdef ONLY_MAIN_THREAD
		return this->Auth(true);
#else
		custom::ThreadSync<bool> threadSync;
		Asio::Context& context = this->mSocket->GetContext();
		asio::post(context, [this, &threadSync]
		{
			if (this->Auth(true))
			{
				threadSync.SetResult(true);
				return;
			}
			threadSync.SetResult(false);
		});
		return threadSync.Wait();
#endif
	}

	Client::Client(int id, Component* component, mongo::Config config, Asio::Context& io)
			: tcp::Client(1024 * 1024), mClientId(id), mComponent(component), mConfig(std::move(config)), mMainContext(io)
	{
		this->mRequest = nullptr;
		this->mResponse = nullptr;
	}

	Client::Client(int id, mongo::Config config, Asio::Context& io)
			: tcp::Client(0), mClientId(id), mComponent(nullptr), mConfig(std::move(config)), mMainContext(io)
	{
		this->mRequest = nullptr;
		this->mResponse = nullptr;
	}

	void Client::OnSendMessage(size_t size)
	{
		this->ReadLength(sizeof(Head));
	}

	void Client::OnSendMessage(const Asio::Code& code)
	{
		this->Connect();
		LOG_ERROR("client:{} send => {}", this->mClientId, code.message())
	}

	bool Client::AuthBySha1(const std::string& user, const std::string& db, const std::string& pwd)
	{
		std::string nonce = _bson::base64::encode(help::Str::RandomString(8));
		std::string firstBare = fmt::format("n={0},r={1}", this->mConfig.user, nonce);
		std::unique_ptr<mongo::Request> request1 = std::make_unique<mongo::Request>();

		std::string payload = _bson::base64::encode(fmt::format("n,,{0}", firstBare));

		request1->dataBase = db;
		request1->header.requestID = 1;
		request1->document.Add("saslStart", 1);
		request1->document.Add("autoAuthorize", 1);

		request1->document.Add("mechanism", mongo::auth::SCRAM_SHA1);

		request1->document.Add("payload", payload);

		std::unique_ptr<Response> response1 = this->SyncSendMongoCommand(request1);
		if (response1 == nullptr)
		{
			return false;
		}

		int conversationId = 0;
		std::string server_first;
		const bson::r::Document & document1 = response1->document;
		if (!document1.Get("payload", server_first) || !document1.Get("conversationId", conversationId))
		{
			return false;
		}

		std::string parsedSource = _bson::base64::decode(server_first);

		std::vector<std::string> ret;
		help::Str::Split(parsedSource, ',', ret);

		std::string salt(ret[1].c_str() + 2, ret[1].size() - 2);
		std::string rnonce(ret[0].c_str() + 2, ret[0].size() - 2);
		int iterations = std::stoi(std::string(ret[2].c_str() + 2, ret[2].size() - 2));

		std::string without_proof = "c=biws,r=" + rnonce;
		std::string pbkdf2_key = SumHex(fmt::format("{0}:mongo:{1}", user, pwd));
		std::string salted_pass = SaltPasswordBySha1(pbkdf2_key,
				_bson::base64::decode(salt), iterations);


		std::string client_key = help::Sha1::GetHMacHash(salted_pass, "Client Key");
		std::string stored_key = help::Sha1::GetHash(client_key);

		std::string auth_msg = firstBare + ',' + parsedSource + ',' + without_proof;
		std::string client_sin = help::Sha1::GetHMacHash(stored_key, auth_msg);
		std::string client_key_xor_sig = help::Sha1::XorString(client_key, client_sin);
		std::string client_proof = std::string("p=") + _bson::base64::encode(client_key_xor_sig);
		std::string client_final = _bson::base64::encode(without_proof + "," + client_proof);
		std::string server_key = help::Sha1::GetHMacHash(salted_pass, "Server Key");
		std::string server_sig = _bson::base64::encode(help::Sha1::GetHMacHash(server_key, auth_msg));

		std::unique_ptr<mongo::Request> request2 = std::make_unique<mongo::Request>();
		{
			request2->dataBase = db;
			request2->header.requestID = 1;
			request2->document.Add("saslContinue", 1);
			request2->document.Add("conversationId", conversationId);
			request2->document.Add("payload", client_final);
		}
		std::unique_ptr<Response> response2 = this->SyncSendMongoCommand(request2);
		if (response2 == nullptr)
		{
			return false;
		}
		parsedSource.clear();
		const bson::r::Document & document2 = response2->document;
		if (!document2.Get("payload", parsedSource))
		{
			return false;
		}
		if(document2.GetBool("done"))
		{
			return true;
		}
		std::unique_ptr<Request> request3 = std::make_unique<Request>();
		{
			request3->dataBase = db;
			request3->header.requestID = 1;
			request3->document.Add("saslContinue", 1);
			request3->document.Add("conversationId", conversationId);
			request3->document.Add("payload", "");
		}

		std::unique_ptr<Response> response3 = this->SyncSendMongoCommand(request3);

		if (response3 == nullptr)
		{
			return false;
		}
		return response3->document.IsOk() && response3->document.GetBool("done");
	}

#ifdef __ENABLE_OPEN_SSL__

	bool Client::AuthBySha256(const std::string& user, const std::string& db, const std::string& pwd)
	{
		std::string nonce = _bson::base64::encode(help::Str::RandomString(16));
		std::string firstBare = fmt::format("n={0},r={1}", user, nonce);
		std::unique_ptr<mongo::Request> request1 = std::make_unique<mongo::Request>();

		std::string payload = _bson::base64::encode(fmt::format("n,,{0}", firstBare));

		request1->dataBase = db;
		request1->header.requestID = 1;
		request1->document.Add("saslStart", 1);
		request1->document.Add("autoAuthorize", 1);
		request1->document.Add("mechanism", "SCRAM-SHA-256");
		request1->document.Add("payload", payload);

		std::unique_ptr<Response> response1 = this->SyncSendMongoCommand(request1);
		if (response1 == nullptr)
		{
			return false;
		}

		int conversationId = 0;
		std::string server_first;
		const bson::r::Document & document1 = response1->document;
		if (!document1.Get("payload", server_first) || !document1.Get("conversationId", conversationId))
		{
			return false;
		}

		std::string parsedSource = _bson::base64::decode(server_first);
		std::vector<std::string> ret;
		help::Str::Split(parsedSource, ',', ret);
		if (ret.size() < 3)
		{
			return false;
		}

		std::string salt(ret[1].c_str() + 2, ret[1].size() - 2);
		std::string rnonce(ret[0].c_str() + 2, ret[0].size() - 2);
		int iterations = std::stoi(std::string(ret[2].c_str() + 2, ret[2].size() - 2));

		if (rnonce.find(nonce) != 0)
		{
			return false;
		}

		std::string without_proof = "c=biws,r=" + rnonce; // biws = base64("n,,")
		std::string salted_pass = SaltPasswordBySha256(pwd, _bson::base64::decode(salt), iterations);

		std::string auth_msg = firstBare + ',' + parsedSource + ',' + without_proof;

		unsigned char client_key[SHA256_DIGEST_LENGTH];
		HMAC(EVP_sha256(), salted_pass.c_str(), salted_pass.size(),
				(const unsigned char*)"Client Key", strlen("Client Key"), client_key, nullptr);

		unsigned char server_key[SHA256_DIGEST_LENGTH];
		HMAC(EVP_sha256(), salted_pass.c_str(), salted_pass.size(),
				(const unsigned char*)"Server Key", strlen("Server Key"), server_key, nullptr);

		std::string stored_key(SHA256_DIGEST_LENGTH, '\0');
		SHA256(client_key, SHA256_DIGEST_LENGTH, (unsigned char*)stored_key.data());

		std::string client_sig(SHA256_DIGEST_LENGTH, '\0');
		HMAC(EVP_sha256(), stored_key.c_str(), stored_key.size(),
				(const unsigned char*)auth_msg.c_str(), auth_msg.size(),
				(unsigned char*)client_sig.data(), nullptr);

		std::string client_key_xor_sig = help::Sha1::XorString(std::string((char*)client_key, SHA256_DIGEST_LENGTH),
				client_sig);
		std::string client_proof = "p=" + _bson::base64::encode(client_key_xor_sig);
		std::string client_final = _bson::base64::encode(without_proof + "," + client_proof);

		std::unique_ptr<mongo::Request> request2 = std::make_unique<mongo::Request>();
		{
			request2->dataBase = db;
			request2->header.requestID = 1;
			request2->document.Add("saslContinue", 1);
			request2->document.Add("conversationId", conversationId);
			request2->document.Add("payload", client_final);
		}

		std::unique_ptr<Response> response2 = this->SyncSendMongoCommand(request2);
		if (response2 == nullptr)
		{
			CONSOLE_LOG_ERROR("saslContinue failed: response is null");
			return false;
		}

		std::string parsedSource2;
		const bson::r::Document & document2 = response2->document;
		if (!document2.Get("payload", parsedSource2))
		{
			return false;
		}
		if(document2.GetBool("done"))
		{
			return true;
		}
		std::unique_ptr<Request> request3 = std::make_unique<Request>();
		{
			request3->dataBase = db;
			request3->header.requestID = 1;
			request3->document.Add("saslContinue", 1);
			request3->document.Add("conversationId", conversationId);
			request3->document.Add("payload", "");
		}

		std::unique_ptr<Response> response3 = this->SyncSendMongoCommand(request3);
		if (response3 == nullptr)
		{
			return false;
		}
		return response3->document.IsOk() && response3->document.GetBool("done");
	}

#endif

	void Client::OnReadError(const Asio::Code& code)
	{
		this->Connect(3);
		LOG_ERROR("client:{} read => {}", this->mClientId, code.message())
	}

	void Client::OnReceiveMessage(std::istream& is, size_t size, const Asio::Code&)
	{
		if (this->mResponse == nullptr)
		{
			this->mResponse = std::make_unique<mongo::Response>(this->mRequest->cmd);
		}
		int length = this->mResponse->OnRecvMessage(is, size);
		if (length > 0)
		{
			this->ReadLength(length);
			return;
		}
		this->OnResponse(XCode::Ok, this->mRequest, this->mResponse);
	}

	void Client::OnResponse(int code)
	{
		this->StopTimer();
		this->ClearBuffer();
		if (this->mRequest != nullptr)
		{
			int id = this->mRequest->header.requestID;
			this->mResponse = std::make_unique<mongo::Response>(id, this->mRequest->cmd);
			this->OnResponse(code, this->mRequest, this->mResponse);
		}
	}

	void Client::OnResponse(int code, std::unique_ptr<Request>& request, std::unique_ptr<Response>& response)
	{
		this->StopTimer();
		this->ClearBuffer();
		if (this->mComponent == nullptr)
		{
			this->mRequest.reset();
			this->mResponse.reset();
			return;
		}
		if (request->GetRpcId() != response->RpcId())
		{
			response->SetRpcId(request->GetRpcId());
		}
		mongo::Request* req = request.release();
		mongo::Response* resp = response.release();
#ifdef ONLY_MAIN_THREAD
		this->mComponent->OnMessage(this->mClientId, req, resp);
#else

		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(this->mMainContext, [this, self, req, id = this->mClientId, resp]
		{
			this->mComponent->OnMessage(id, req, resp);
		});
#endif
	}

	void Client::Send(std::unique_ptr<Request>& request)
	{
#ifdef ONLY_MAIN_THREAD
		this->mRequest = std::move(request);
		if(this->mRequest->dataBase.empty())
		{
			this->mRequest->dataBase = this->mConfig.db;
		}
		this->Write(*this->mRequest);
#else
		Asio::Context& context = this->mSocket->GetContext();
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(context, [this, self, req = request.release()]
		{
			if(req->dataBase.empty())
			{
				req->dataBase = this->mConfig.db;
			}
			this->mRequest.reset(req);
			this->Write(*this->mRequest);
		});
#endif
	}

	bool Client::SyncSend(const std::unique_ptr<Request>& request, mongo::Response& response)
	{
		if (!this->SendSync(*request))
		{
			return false;
		}
		size_t readSize = 0;
		if (!this->RecvSync(sizeof(Head), readSize))
		{
			return false;
		}
		std::istream readStream1(&this->mRecvBuffer);
		{
			int count = response.OnRecvMessage(readStream1, readSize);
			if (!this->RecvSync(count, readSize))
			{
				return false;
			}
			response.OnRecvMessage(readStream1, readSize);
		}
		return true;
	}

	std::unique_ptr<mongo::Response> Client::SyncMongoCommand(std::unique_ptr<Request>& request)
	{
#ifdef ONLY_MAIN_THREAD
		return this->SyncSendMongoCommand(request);
#else
		custom::ThreadSync<bool> threadSync;
		std::unique_ptr<mongo::Response> response;
		Asio::Socket& sock = this->mSocket->Get();
		const Asio::Executor& executor = sock.get_executor();
		asio::post(executor, [this, &request, &response, &threadSync]
		{
			response = this->SyncSendMongoCommand(request);
			threadSync.SetResult(true);
		});
		threadSync.Wait();
		return response;
#endif
	}

	std::unique_ptr<Response> Client::SyncSendMongoCommand(const std::unique_ptr<Request>& request)
	{
		if (!this->SendSync(*request))
		{
			return nullptr;
		}
		return this->ReadResponse(request->cmd);
	}

	std::unique_ptr<Response> Client::ReadResponse(const std::string & cmd)
	{
		size_t readSize = 0;
		if (!this->RecvSync(sizeof(Head), readSize))
		{
			return nullptr;
		}
		std::istream readStream1(&this->mRecvBuffer);
		std::unique_ptr<Response> response = std::make_unique<Response>(cmd);
		{
			int count = response->OnRecvMessage(readStream1, readSize);
			if (!this->RecvSync(count, readSize))
			{
				return response;
			}
			response->OnRecvMessage(readStream1, readSize);
		}
		return response;
	}

	void Client::OnConnect(const Asio::Code& code, int count)
	{
		if (code.value() != Asio::OK)
		{
			LOG_ERROR("client:{} connect =>{}", this->mClientId, code.message())
			if (count < this->mConfig.conn_count)
			{
				this->Connect(5);
				return;
			}
		}
		else if (this->Auth(false))
		{
			LOG_DEBUG("client:{} connect ok", this->mClientId)
			if (this->mRequest != nullptr)
			{
				this->Write(*this->mRequest);
			}
			return;
		}
		if (this->mComponent != nullptr)
		{
			int id = this->mClientId;
			asio::post(this->mMainContext, [this, self = this->shared_from_this(), id]
			{
				this->mComponent->OnClientError(id, XCode::NetConnectFailure);
				if (this->mRequest != nullptr)
				{
					this->mComponent->OnSendFailure(id, this->mRequest.release());
				}
			});
		}
	}

	bool Client::Auth(bool connect)
	{
		if (connect)
		{
			Asio::Code code;
			if (!this->ConnectSync(code))
			{
				return false;
			}
		}
		bool result = true;
		if (!this->mConfig.password.empty())
		{
			if (this->mConfig.mechanism == mongo::auth::SCRAM_SHA1)
			{
				if (!this->AuthBySha1(this->mConfig.user, this->mConfig.db, this->mConfig.password))
				{
					result = this->AuthBySha1(this->mConfig.user, "admin", this->mConfig.password);
				}
			}
#ifdef __ENABLE_OPEN_SSL__
			else if (this->mConfig.mechanism == mongo::auth::SCRAM_SHA256)
			{
				if (!this->AuthBySha256(this->mConfig.user, this->mConfig.db, this->mConfig.password))
				{
					result = this->AuthBySha256(this->mConfig.user, "admin", this->mConfig.password);
				}
			}
#endif
			else
			{
				result = false;
			}
		}
		std::unique_ptr<mongo::Request> request = std::make_unique<mongo::Request>();
		{
			request->dataBase = "admin";
			request->document.Add("serverStatus", 1);
		}
		std::unique_ptr<mongo::Response> response = this->SyncSendMongoCommand(request);
		{
			response->document.Get("host", this->mInfo.host);
			response->document.Get("version", this->mInfo.version);
		}
		std::string res = response->ToString();
		if (result && this->mRequest == nullptr && this->mComponent != nullptr)
		{
			std::shared_ptr<tcp::Client> self = this->shared_from_this();
			asio::post(this->mMainContext, [self, this, id = this->mClientId]()
			{
				this->mComponent->OnConnectOK(id);
			});
		}
		return result;
	}

}