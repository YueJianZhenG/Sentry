//
// Created by yjz on 2023/5/17.
//

#ifndef _ACTOR_H_
#define _ACTOR_H_
#include"Entity/Unit/Unit.h"
#include"Rpc/Client/Message.h"
#include"Proto/Include/Message.h"
struct lua_State;
namespace Tendo
{
	class Actor : public Unit
	{
	 public:
		explicit Actor(long long id);
	 public:
		bool LateAwake() final;
		int Send(const std::string& func);
		int Send(const std::string& func, const pb::Message& request);
	 public:
		int Call(const std::string & func);
		int Call(const std::string & func, const pb::Message & request);
		int Call(const std::string & func, std::shared_ptr<pb::Message> response);
		int Call(const std::string & func, const pb::Message & request, std::shared_ptr<pb::Message> response);
	public:
		int LuaSend(lua_State * lua, const std::string & func, const std::shared_ptr<Msg::Packet> & message);
		int LuaCall(lua_State * lua, const std::string & func, const std::shared_ptr<Msg::Packet> & message);
		int MakeMessage(lua_State * lua, int idx, const std::string & func, std::shared_ptr<Msg::Packet> & message) const;
	public:
		long long GetActorId() const { return this->GetUnitId(); }
		virtual int GetAddress(const std::string & func, std::string & addr) = 0;
	protected:
		virtual bool OnInit() = 0;
		std::shared_ptr<Msg::Packet> Make(const std::string & func, const pb::Message * message) const;
	protected:
		class InnerNetComponent * mNetComponent;
	private:
		long long mLastTime;
	};
}

#endif //_ACTOR_H_
