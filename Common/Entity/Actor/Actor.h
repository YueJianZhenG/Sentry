//
// Created by yjz on 2023/5/17.
//

#ifndef _ACTOR_H_
#define _ACTOR_H_
#include"Entity/Unit/Entity.h"
#include"Rpc/Client/Message.h"
#include"Proto/Include/Message.h"
struct lua_State;
namespace Tendo
{
	class Actor : public Entity
	{
	 public:
		explicit Actor(long long id, std::string  name);
	 public:
		bool LateAwake() final;
		int Send(const std::string& func);
		const std::string & Name() const { return this->mName; }
		int Send(const std::string& func, const pb::Message& request);
	 public:
		int Call(const std::string & func);
		int Call(const std::string & func, const pb::Message & request);
		int Call(const std::string & func, const std::shared_ptr<pb::Message>& response);
		int Call(const std::string & func, const pb::Message & request, const std::shared_ptr<pb::Message>& response);
	public:
		int LuaSend(lua_State * lua, const std::string & func, const std::shared_ptr<Msg::Packet> & message);
		int LuaCall(lua_State * lua, const std::string & func, const std::shared_ptr<Msg::Packet> & message);
		int MakeMessage(lua_State * lua, int idx, const std::string & func, std::shared_ptr<Msg::Packet> & message) const;
	public:
		long long GetActorId() const { return this->GetEntityId(); }
	 public:
		virtual bool OnInit() = 0;
		virtual void OnRegister(std::string * json) = 0;
		virtual bool DecodeFromJson(const std::string & json) { return true;}
		virtual int GetAddress(const std::string & func, std::string & addr) = 0;
	protected:
		virtual void OnWriteRpcHead(const std::string & func, Msg::Head & head) const { }
		std::shared_ptr<Msg::Packet> Make(const std::string & func, const pb::Message * message) const;
	protected:
		class RouterComponent * mRouterComponent;
	private:
		long long mLastTime;
		const std::string mName;
	};
}

#endif //_ACTOR_H_
