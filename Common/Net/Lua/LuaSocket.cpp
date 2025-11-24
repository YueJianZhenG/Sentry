//
// Created by mac on 2022/5/30.
//

#include"LuaSocket.h"
#include"Entity/Actor/App.h"
#include"Util/Tools/String.h"
#include"Lua/Socket/LuaTcpSocket.h"
#include"Async/Lua/LuaWaitTaskSource.h"
#include"Lua/Engine/UserDataParameter.h"


namespace lua
{
	int TcpSock::Listen(lua_State* L)
	{
		try
		{
			int port = (int)luaL_checkinteger(L, 1);
			Asio::Context& io = acs::App::Inst()->GetContext();
			Asio::EndPoint listenAddress(asio::ip::address(), port);
			std::unique_ptr<Asio::Acceptor> acceptor = std::make_unique<Asio::Acceptor>(io);
			{
				acceptor->open(listenAddress.protocol());
				acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true));
			}
			acceptor->bind(listenAddress);
			acceptor->listen();
			Lua::UserDataParameter::UserDataStruct<Asio::Acceptor*>::WritePtr(L, acceptor.release());
			return 1;
		}
		catch (std::system_error& error)
		{
			luaL_error(L, error.what());
			return 0;
		}
	}

	int TcpSock::Accept(lua_State* L)
	{
		Asio::Acceptor* tcpAcceptor = Lua::UserDataParameter::Read<Asio::Acceptor*>(L, 1);
		if (tcpAcceptor == nullptr)
		{
			luaL_error(L, "must userdata");
			return 0;
		}
		lua_pushthread(L);
		tcp::Socket * tcpSocket = new tcp::Socket(acs::App::Inst()->GetContext());
		std::shared_ptr<acs::LuaWaitTaskSource> waitTaskSource = std::make_shared<acs::LuaWaitTaskSource>(L);
		tcpAcceptor->async_accept(tcpSocket->Get(), [tcpSocket, waitTaskSource](const Asio::Code& code)
		{
			if (code.value() == Asio::OK)
			{
				lua::TcpClient* tcpClient = new lua::TcpClient();
				{
					tcpClient->async = true;
					tcpClient->mSocket.reset(tcpSocket);
				}
				waitTaskSource->SetResultPtr<lua::TcpClient>(tcpClient);
				return;
			}
			tcpSocket->Destroy();
			waitTaskSource->SetError(code.message().c_str());
		});
		return waitTaskSource->Await();
	}

	int TcpSock::Send(lua_State* L)
	{
		lua::TcpClient* tcpClient = Lua::UserDataParameter::Read<lua::TcpClient*>(L, 1);
		if(tcpClient == nullptr)
		{
			luaL_error(L, "must userdata => [lua::TcpClient]");
			return 0;
		}
		if(tcpClient->async)
		{
			lua_pushthread(L);
			std::shared_ptr<acs::LuaWaitTaskSource> waitTaskSource = std::make_shared<acs::LuaWaitTaskSource>(L);
			{
				auto callback = [waitTaskSource](const Asio::Code& code, size_t size) {
					if(code.value() == Asio::OK)
					{
						waitTaskSource->SetResults(lua::Status::Ok, size);
						return;
					}
					waitTaskSource->SetResults(lua::Status::SendError, code.message());
				};
				std::ostream os(&tcpClient->mSendBuffer);
				if(lua_isinteger(L, 2))
				{
					size_t size = 0;
					unsigned int session = 0;
					unsigned short opcode = (int)lua_tointeger(L, 2);
					const char* buff = luaL_checklstring(L, 3, &size);
					unsigned short length = sizeof(opcode) + size + sizeof(session);

					os.write((char*)&length, sizeof(length));
					os.write((char*)&session, sizeof(session));
					os.write((char*)&opcode, sizeof(opcode));
					os.write(buff, (std::streamsize)size);
				}
				else
				{
					size_t size = 0;
					const char* buff = luaL_checklstring(L, 2, &size);
					os.write(buff, (std::streamsize)size);
				}

				asio::async_write(tcpClient->mSocket->Get(), tcpClient->mSendBuffer, callback);
			}
			return waitTaskSource->Await();
		}
		else
		{
			size_t size = 0;
			asio::error_code code;
			const char* buff = luaL_checklstring(L, 2, &size);
			size_t count = asio::write(tcpClient->mSocket->Get(), asio::buffer(buff, size), code);
			if(code.value() != Asio::OK)
			{
				luaL_error(L, code.message().c_str());
				return 0;
			}
			lua_pushinteger(L, count);
			return 1;
		}
	}

	int TcpSock::SetTimeout(lua_State* L)
	{
		lua::TcpClient* tcpClient = Lua::UserDataParameter::Read<lua::TcpClient*>(L, 1);
		if(tcpClient == nullptr)
		{
			luaL_error(L, "must userdata => [lua::TcpClient]");
			return 0;
		}
		Asio::Context & io = tcpClient->mSocket->GetContext();
		tcpClient->mTimer = std::make_unique<Asio::Timer>(io);
		tcpClient->timeout = (int)(luaL_checknumber(L, 2) * 1000);
		return 0;
	}

	int TcpSock::Read(lua_State* L)
	{
		lua::TcpClient* tcpClient = Lua::UserDataParameter::Read<lua::TcpClient*>(L, 1);
		if(tcpClient == nullptr)
		{
			luaL_error(L, "must userdata => [lua::TcpClient]");
			return 0;
		}
		if(tcpClient->async) //异步模式
		{
			lua_pushthread(L);
			Asio::Context & io = tcpClient->mSocket->GetContext();
			std::shared_ptr<acs::LuaWaitTaskSource> waitTaskSource = std::make_shared<acs::LuaWaitTaskSource>(L);
			auto callback = [waitTaskSource, tcpClient](const Asio::Code& code, size_t size)
			{
				if(tcpClient->mTimer != nullptr)
				{
					asio::error_code errorCode;
					tcpClient->mTimer->cancel(errorCode);
				}
				if (code.value() == Asio::OK || code == asio::error::eof)
				{
					std::string result;
					result.resize(size);
					char * buff = const_cast<char*>(result.c_str());
					{
						std::istream istream(&tcpClient->mBuffer);
						size_t count = istream.readsome(buff, size);
					}
					waitTaskSource->SetResults(lua::Status::Ok, result);
				}
				else
				{
					waitTaskSource->SetResults(lua::Status::ReadError, code.message());
				}
			};

			switch(lua_type(L, 2))
			{
				case LUA_TSTRING:
				{
					const char * delim = lua_tostring(L, 2);
					asio::async_read_until(tcpClient->mSocket->Get(), tcpClient->mBuffer, delim, callback);
					break;
				}
				case LUA_TNUMBER:
				{
					int count = lua_tointeger(L, 2);
					asio::async_read(tcpClient->mSocket->Get(), tcpClient->mBuffer, asio::transfer_exactly(count), callback);
					break;
				}
				default:
				{
					asio::async_read(tcpClient->mSocket->Get(), tcpClient->mBuffer, asio::transfer_at_least(1), callback);
					break;
				}
			}
			if(tcpClient->mTimer != nullptr)
			{
				int second = tcpClient->timeout;
				tcpClient->mTimer->expires_after(std::chrono::milliseconds (second));
				tcpClient->mTimer->async_wait([callback, waitTaskSource](const Asio::Code & code)
				{
					if(code != asio::error::operation_aborted)
					{
						waitTaskSource->SetResults(lua::Status::Timeout, "read message time out");
					}
				});
			}
			return waitTaskSource->Await();
		}
		else
		{
			asio::error_code code;
			size_t size = asio::read(tcpClient->mSocket->Get(), tcpClient->mBuffer, asio::transfer_all(), code);
			if (code.value() == Asio::OK || code == asio::error::eof)
			{
				std::string result;
				result.resize(size);
				std::istream istream(&tcpClient->mBuffer);
				char* buff = const_cast<char*>(result.c_str());
				size_t count = istream.readsome(buff, (int)size);
				lua_pushinteger(L, count);
				lua_pushlstring(L, result.c_str(), count);
				return 2;
			}
			else
			{
				lua_pushinteger(L, 0);
				lua_pushstring(L, code.message().c_str());
				return 2;
			}
		}
	}

	int TcpSock::Close(lua_State* L)
	{
		lua::TcpClient* tcpClient = Lua::UserDataParameter::Read<lua::TcpClient*>(L, 1);
		if(tcpClient == nullptr)
		{
			luaL_error(L, "must userdata => [lua::TcpClient]");
			return 0;
		}
		tcpClient->mSocket->Close();
		if(tcpClient->mTimer != nullptr)
		{
			asio::error_code code;
			tcpClient->mTimer->cancel(code);
		}
		return 0;
	}

	int TcpSock::Query(lua_State* L)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 1, &count);

		std::string host(str, count);
		std::vector<std::string> result;
		if (help::Str::IsIpAddress(host))
		{
			result.emplace_back(host);
		}
		else
		{
			Asio::Context& io = acs::App::Inst()->GetContext();

			Asio::Code code;
			Asio::Resolver resolver(io);
			Asio::ResolverQuery query(host, "");
			auto iter = resolver.resolve(query, code);
			if(code.value() != Asio::OK)
			{
				LOG_ERROR("query : {} =>{}", host, code.message());
				return 0;
			}
			Asio::Resolver::iterator end;
			for (; iter != end; iter++)
			{
				result.emplace_back(iter->endpoint().address().to_string());
			}
		}
		lua_newtable(L);
		int top = lua_gettop(L);
		for (size_t index = 0; index < result.size(); index++)
		{
			lua_pushinteger(L, index + 1);
			const std::string& str = result.at(index);
			lua_pushlstring(L, str.c_str(), str.size());
			lua_settable(L, -3);
		}
		lua_settop(L, top);
		return 1;
	}

	int TcpSock::Connect(lua_State* L)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 1, &count);
		const int port = (int)luaL_checkinteger(L, 2);

		std::string host(str, count);
		Asio::Context& io = acs::App::Inst()->GetContext();
		std::unique_ptr<lua::TcpClient> tcpClient = std::make_unique<lua::TcpClient>();
		try
		{
			if(lua_isboolean(L, 3))
			{
				tcpClient->async = (bool)lua_toboolean(L, 3);
			}
			tcpClient->mSocket = std::make_unique<tcp::Socket>(io);
			if (help::Str::IsIpAddress(host))
			{
				Asio::Address address = asio::ip::make_address(host);
				{
					Asio::EndPoint endPoint(address, port);
					tcpClient->mSocket->Get().connect(endPoint);
				}
				Lua::UserDataParameter::UserDataStruct<lua::TcpClient*>::WritePtr(L, tcpClient.release());
				return 1;
			}
			Asio::Resolver resolver(io);
			Asio::ResolverQuery query(host, std::to_string(port));
			asio::connect(tcpClient->mSocket->Get(), resolver.resolve(query));
			Lua::UserDataParameter::UserDataStruct<lua::TcpClient*>::WritePtr(L, tcpClient.release());
			return 1;
		}
		catch (const asio::system_error& code)
		{
			LOG_ERROR("connect : {} => {}", host, code.what());
			return 0;
		}
	}
}
