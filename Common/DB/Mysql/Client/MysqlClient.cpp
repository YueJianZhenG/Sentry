//
// Created by yy on 2025/2/10.
//

#include "MysqlClient.h"

#include "Util/Tools/String.h"
#include "Util/File/FileHelper.h"
#include "Core/Thread/ThreadSync.h"
#include "XCode/XCode.h"
#include "Util/File/DirectoryHelper.h"

namespace mysql
{
	Client::Client(int id, mysql::Client::Component* component, mysql::Config config, Asio::Context& io)
			: mClientID(id), tcp::Client(1024 * 1024), mConfig(std::move(config)), mComponent(component),
			  mMain(io)
	{
		this->mIndex = 0;
	}

	int Client::Start(tcp::Socket* socket)
	{
#ifdef ONLY_MAIN_THREAD
		if(socket == nullptr)
		{
			this->Connect(5);
			return XCode::Ok;
		}
		this->SetSocket(socket);
		return this->Auth(true);
#else
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		if (socket == nullptr)
		{
			Asio::Context& context = this->mSocket->GetContext();
			asio::post(context, [self, this]()
			{
				this->mConnectCount = 0;
				this->Connect(5);
			});
			return XCode::Ok;
		}
		this->SetSocket(socket);
		custom::ThreadSync<int> threadSync;
		Asio::Context& context = this->mSocket->GetContext();
		asio::post(context, [this, &threadSync, self]
		{
			int code = this->Auth(true);
			threadSync.SetResult(code);
		});
		return threadSync.Wait();
#endif

	}

	void Client::StartReceive()
	{
		Asio::Context& context = this->mSocket->GetContext();
		asio::post(context, [self = this->shared_from_this(), this]
		{
			this->ReadLength(4);
		});
	}

	bool Client::InvokeCompileSql()
	{
		if(this->mMessage != nullptr)
		{
			return false;
		}
#ifdef ONLY_MAIN_THREAD
		return this->OnCompileSql();
#else
		custom::ThreadSync<bool> threadSync;
		Asio::Context& context = this->mSocket->GetContext();
		asio::post(context, [&threadSync, self = this->shared_from_this(), this]
		{
			threadSync.SetResult(this->OnCompileSql());
		});
		return threadSync.Wait();
#endif

	}

	void Client::Send(std::unique_ptr<Request> request)
	{
		Asio::Context& context = this->mSocket->GetContext();
		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(context, [req = request.release(), this, self]()
		{
			unsigned char index = ++this->mIndex;
			if (index == 255)
			{
				this->mIndex = 0;
			}
			req->SetIndex(0);
			this->mMessage.reset(req);
			this->Send(*this->mMessage);
		});
	}

	bool Client::OnCompileSql()
	{
		std::vector<std::string> filePaths;
		const std::string& dir = this->mConfig.script;
		if (help::dir::GetFilePaths(dir, ".sql", filePaths) > 0)
		{
			for (const std::string& path: filePaths)
			{
				std::vector<std::string> compileSql;
				if (!help::fs::ReadTxtFile(path, compileSql))
				{
					return false;
				}
				for (const std::string& sql: compileSql)
				{
					this->SyncRun(sql, this->mResponse);
					if (this->mResponse.HasError())
					{
						LOG_ERROR("{}", sql);
						LOG_ERROR("{}", this->mResponse.GetBuffer());
						return false;
					}
				}
			}
		}
		return true;
	}

	void Client::Send(mysql::Request& request)
	{
		//const std::string & sql = request.Get();
		// if(!sql.empty())
		// {
		// 	this->SyncRun(sql, this->mResponse);
		// }
		this->Write(request);
	}

	bool Client::SyncReadOnePacket(mysql::Response& response)
	{
		size_t size = 0;
		response.Clear();
		if (!this->RecvSync(4, size))
		{
			return false;
		}
		std::istream stream(&this->mRecvBuffer);
		int len = response.OnRecvMessage(stream, size);
		if (len <= 0)
		{
			return false;
		}
		if (!this->RecvSync(len, size))
		{
			return false;
		}
		response.OnRecvMessage(stream, size);
		this->mIndex = response.GetIndex();
		return true;
	}

	int Client::Auth(bool connect)
	{
		if (connect)
		{
			Asio::Code error;
			if (!this->ConnectSync(error))
			{
				return XCode::NetConnectFailure;
			}
		}
		if (!this->SyncReadOnePacket(this->mResponse))
		{
			return XCode::NetReadFailure;
		}
		if(this->mResponse.GetPackageCode() == mysql::PACKAGE_OK)
		{
			CONSOLE_LOG_ERROR("{}", this->mResponse.GetBuffer());
			return XCode::Failure;
		}
		if (!this->mHandshake.OnDecode(this->mResponse))
		{
			return XCode::PasswordAuthFail;
		}
		std::string message;
		uint32_t capability_flags = this->mHandshake.capability_flags;
		mysql::LoginRequest loginRequest;
		{
			loginRequest.user = this->mConfig.user;
			//loginRequest.database = this->mConfig.db;
			loginRequest.salt = this->mHandshake.salt;
			loginRequest.charset = mysql::config::charset;
			loginRequest.password = this->mConfig.password;
			loginRequest.authPlugin = this->mHandshake.auth_plugin_name;
			loginRequest.enable_compression = (capability_flags & mysql::client_flag::CLIENT_COMPRESS) != 0;
		}
		int code = this->Login(loginRequest);
		if (code != XCode::Ok)
		{
			return code;
		}
		if (!this->InitDataBase())
		{
			return XCode::DataBaseCreateFailure;
		}
		return XCode::Ok;
	}

	bool Client::SendPacket(unsigned char* message, size_t size, unsigned char sequence_id)
	{
		std::vector<uint8_t> final_packet;
		std::array<uint8_t, 3> len_packet = mysql::encode(size);
		{
			final_packet.emplace_back(len_packet[0]);
			final_packet.emplace_back(len_packet[1]);
			final_packet.emplace_back(len_packet[2]);
			final_packet.emplace_back(sequence_id);
		}
		for(size_t index = 0; index < size; index++)
		{
			final_packet.emplace_back(message[index]);
		}
		return this->SendSync((char*)final_packet.data(), final_packet.size());
	}

//	int Client::OnMoreAuth(unsigned char status)
//	{
//		const std::string & password = this->mConfig.password;
//		switch(status)
//		{
//			case 0x03: //发送明文密码
//			{
//				std::string plain = password + '\0';
//				unsigned char id = this->mResponse.GetIndex() + 1;
//				if(!this->SendPacket((unsigned char*)plain.data(), plain.size(), id))
//				{
//					return XCode::SendMessageFail;
//				}
//				break;
//			}
//			case 0x04: //rsa加密
//			{
//				std::vector<uint8_t> requestPubkey { 0x02 };
//				unsigned char id = this->mResponse.GetIndex() + 1;
//				if(!this->SendPacket(requestPubkey.data(), requestPubkey.size(), id))
//				{
//					return XCode::SendMessageFail;
//				}
//				if (!this->SyncReadOnePacket(this->mResponse))
//				{
//					return XCode::NetReadFailure;
//				}
//				std::string pubkey_data = this->mResponse.GetBuffer();
//				const std::string scramble = this->mHandshake.salt;
//				BIO* bio = BIO_new_mem_buf(pubkey_data.data(), pubkey_data.size());
//				RSA* rsa = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
//				BIO_free(bio);
//
//				if (!rsa) {
//					return XCode::PasswordAuthFail;
//				}
//
//				std::string plain = password + '\0';
//				for (size_t i = 0; i < plain.size(); ++i) {
//					plain[i] ^= scramble[i % scramble.size()];
//				}
//
//				unsigned char encrypted[256]; // 2048位公钥
//				int len = RSA_public_encrypt(
//						plain.size(), (const unsigned char*)plain.data(),
//						encrypted, rsa, RSA_PKCS1_OAEP_PADDING);
//				if (len <= 0) {
//					return XCode::PasswordAuthFail;
//				}
//				RSA_free(rsa);
//				id = this->mResponse.GetIndex() + 1;
//				if(!this->SendPacket(encrypted, len, id))
//				{
//					return XCode::SendMessageFail;
//				}
//				break;
//			}
//			default:
//				return XCode::Failure;
//		}
//		return XCode::Ok;
//	}

	int Client::Login(const mysql::LoginRequest& request)
	{
		std::vector<uint8_t> packet;
		if (request.Encode(packet) != XCode::Ok)
		{
			CONSOLE_LOG_ERROR("{}", request.authPlugin);
			return XCode::AuthPluginNonsupport;
		}
		uint8_t sequence_id = 1; // 登录包的 sequence_id 必须是 1
		if(!this->SendPacket(packet.data(), packet.size(), sequence_id))
		{
			return XCode::SendMessageFail;
		}
		unsigned char code = mysql::PACKAGE_OK;
		do
		{
			if (!this->SyncReadOnePacket(this->mResponse))
			{
				return XCode::NetReadFailure;
			}
			code = this->mResponse.GetPackageCode();
			if(code == mysql::PACKAGE_ERR)
			{
				CONSOLE_LOG_ERROR("{}", this->mResponse.GetBuffer())
				return XCode::PasswordAuthFail;
			}

		} while (code == mysql::PACKAGE_MORE);
		return code == mysql::PACKAGE_OK ? XCode::Ok : XCode::Failure;
	}

	bool Client::SyncRun(const std::string& sql, mysql::Response& response)
	{
		mysql::Request request(sql);
		if (!this->SendSync(request))
		{
			return false;
		}
		if (!this->SyncReadOnePacket(response))
		{
			return false;
		}
		return response.IsOk();
	}

	bool Client::InitDataBase()
	{
		if (this->mConfig.db.empty())
		{
			return true;
		}
		// 创建数据库 并切换
		std::string sql1 = fmt::format("USE {};", this->mConfig.db);
		std::string sql2 = fmt::format("CREATE DATABASE IF NOT EXISTS {};", this->mConfig.db);
		if (this->SyncRun(sql2, this->mResponse))
		{
			if (!this->SyncRun(sql1, this->mResponse))
			{
				return false;
			}
		}
		return true;
	}

	void Client::OnSendMessage(size_t size)
	{
		this->ReadLength(4);
	}

	void Client::OnConnect(const Asio::Code& code, int count)
	{
		if (code.value() != Asio::OK)
		{
			LOG_ERROR("client:{} connect => {}", this->mClientID, count)
			if (count <= this->mConfig.conn_count)
			{
				this->Connect(10);
				return;
			}
		}
		else if (this->Auth(false) == XCode::Ok)
		{
			LOG_DEBUG("client:{} connect ok", this->mClientID)
			this->OnCompileSql();
			if (this->mMessage == nullptr)
			{
				std::shared_ptr<tcp::Client> self = this->shared_from_this();
				asio::post(this->mMain, [self, this, id = this->mClientID]()
				{
					this->mComponent->OnConnectOK(id);
				});
			}
			else
			{
				this->Send(*this->mMessage);
			}
			return;
		}
#ifdef ONLY_MAIN_THREAD
		int id = this->mClientID;
		this->mComponent->OnClientError(id, XCode::NetConnectFailure);
		if (this->mMessage != nullptr)
		{
			this->mComponent->OnSendFailure(id, this->mMessage.release());
		}
#else
		int id = this->mClientID;
		asio::post(this->mMain, [this, self = this->shared_from_this(), id]
		{
			this->mComponent->OnClientError(id, XCode::NetConnectFailure);
			if (this->mMessage != nullptr)
			{
				this->mComponent->OnSendFailure(id, this->mMessage.release());
			}
		});
#endif

	}

	void Client::OnSendMessage(const Asio::Code& code)
	{
		this->Connect(10);
		LOG_ERROR("client:{} send error => {}", this->mClientID, code.message())
	}

	void Client::OnReadError(const Asio::Code& code)
	{
		if (code != asio::error::eof)
		{
			this->Connect(10);
		}
		LOG_ERROR("client:{} read error => {}", this->mClientID, code.message())
	}

	void Client::OnReceiveLine(std::istream& readStream, size_t size)
	{
		asio::error_code code;
		this->OnReceiveMessage(readStream, size, code);
	}

	std::unique_ptr<mysql::Response> Client::ReadAllPacket()
	{
		this->mBuffer.clear();
		std::unique_ptr<mysql::Response> response = std::make_unique<mysql::Response>();
		{
			this->SyncReadOnePacket(*response);
			unsigned char code = response->GetPackageCode();
			if (code == mysql::PACKAGE_ERR || code == mysql::PACKAGE_EOF || code == mysql::PACKAGE_OK)
			{
				return response;
			}
			unsigned int pos = 0;
			this->mFields.clear();
			unsigned int columnCount = response->DecodeColumnCount(pos);
			for (unsigned int index = 0; index < columnCount; index++)
			{
				pos = 0;
				this->SyncReadOnePacket(this->mResponse);
				{
					mysql::FieldInfo fieldInfo;
					this->mResponse.ReadField(pos, fieldInfo);
					this->mFields.emplace_back(fieldInfo);
				}
			}
			wrap::string<true> json;
			while (columnCount > 0 && this->SyncReadOnePacket(this->mResponse))
			{
				code = this->mResponse.GetPackageCode();
				const std::string & buffer = this->mResponse.GetBuffer();
				if ((code == mysql::PACKAGE_OK && buffer.empty()) || code == mysql::PACKAGE_EOF)
				{
					break;
				}
				json::w::Document document;
				this->OnTextMessage(document);

				if(document.Serialize(json))
				{
					size_t size = json.size();
					const char * str = json.c_str();
					response->contents.emplace_back(str, size);
				}
			}
		}
		return response;
	}

	void Client::OnTextMessage(json::w::Document& document)
	{
		unsigned int pos = 0;
		for (const mysql::FieldInfo& fieldInfo: this->mFields)
		{
			std::string value = this->mResponse.ReadString(pos);
			switch (fieldInfo.type)
			{
				case mysql::field::MYSQL_TYPE_TINY:
				{
					int number = 0;
					if (!value.empty())
					{
						number = std::stoi(value);
					}
					document.Add(fieldInfo.name.c_str(), number == 1);
					break;
				}
				case mysql::field::MYSQL_TYPE_SHORT:
				case mysql::field::MYSQL_TYPE_INT24:
				case mysql::field::MYSQL_TYPE_YEAR:
				case mysql::field::MYSQL_TYPE_LONG:
				case mysql::field::MYSQL_TYPE_LONGLONG:
				case mysql::field::MYSQL_TYPE_NEWDECIMAL:
				{
					long long number = 0;
					if (!value.empty())
					{
						number = std::stoll(value);
					}
					document.Add(fieldInfo.name.c_str(), number);
					break;
				}
				case mysql::field::MYSQL_TYPE_FLOAT:
				case mysql::field::MYSQL_TYPE_DOUBLE:
				{
					double number = 0;
					if (!value.empty())
					{
						number = std::stod(value);
					}
					document.Add(fieldInfo.name.c_str(), number);
					break;
				}
				case mysql::field::MYSQL_TYPE_JSON:
				{
					document.AddObject(fieldInfo.name.c_str(), value);
					break;
				}
				case mysql::field::MYSQL_TYPE_NULL:
				{
					document.AddNull(fieldInfo.name.c_str());
					break;
				}
				default:
					document.Add(fieldInfo.name.c_str(), value);
					break;
			}
		}
	}

	void Client::OnReceiveMessage(std::istream& readStream, size_t size, const asio::error_code&)
	{
		unsigned int affectedCount = 0;
		std::unique_ptr<mysql::Response> response = this->ReadAllPacket();
		if (this->mMessage != nullptr && this->mMessage->Count() > 1)
		{
			for (size_t index = 0; index < this->mMessage->Count() - 1; index++)
			{
				std::unique_ptr<mysql::Response> response1 = this->ReadAllPacket();
				if(response1->GetPackageCode() == mysql::PACKAGE_ERR)
				{
					response->error.emplace_back(response1->GetBuffer());
				}
				else
				{
					if (response1->ok.mAffectedRows > 0)
					{
						affectedCount += response1->ok.mAffectedRows;
					}
					if(!response1->contents.empty())
					{
						response->contents.insert(response->contents.begin(),
								response1->contents.begin(), response1->contents.end());
					}
				}
			}
		}
		if(response->GetPackageCode() == mysql::PACKAGE_ERR)
		{
			response->error.emplace_back(response->GetBuffer());
		}
		if(this->mMessage->IsEnableCommit())
		{
			std::string sql = response->error.empty()
						? "COMMIT" : "ROLLBACK";
			this->SyncRun(sql, this->mResponse); //有错误就回滚事务
		}
		response->ok.mAffectedRows += affectedCount;
		mysql::Request* req = nullptr;
		if (this->mMessage != nullptr)
		{
			req = this->mMessage.release();
		}
#ifdef ONLY_MAIN_THREAD
		this->mComponent->OnMessage(this->mClientID, req, response.release());
#else

		std::shared_ptr<tcp::Client> self = this->shared_from_this();
		asio::post(this->mMain, [this, self, req, resp = response.release()]
		{
			this->mComponent->OnMessage(this->mClientID, req, resp);
		});
#endif

	}
}
