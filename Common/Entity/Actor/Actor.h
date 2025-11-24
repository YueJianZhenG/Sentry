//
// Created by yjz on 2023/5/17.
//

#ifndef APP_ACTOR_H
#define APP_ACTOR_H
#include"Entity/Unit/Entity.h"
#include"Rpc/Common/Message.h"
struct lua_State;
namespace acs
{
	class Actor : public Entity
	{
	 public:
		explicit Actor(long long id, std::string name);
	 public:
		bool LateAwake() final;
		int Send(const std::string& func) const;
		const std::string & Name() const { return this->mName; }
		int Send(std::unique_ptr<rpc::Message>& message) const;
		int Send(const std::string& func, const pb::Message& request) const;
	 public:
		int Call(const std::string & func) const;
		int Call(std::unique_ptr<rpc::Message>& request);
		int Call(const std::string & func, pb::Message * response) const;
		int Call(const std::string & func, const pb::Message & request) const;
		int Call(const std::string & func, const pb::Message & request, pb::Message * response);
	public:
		template<typename T>
		inline int Call(const std::string & func, std::unique_ptr<T> & response);
		template<typename T>
		inline int Call(const std::string & func, const pb::Message & request, std::unique_ptr<T> & response);
	public:
		int Send(const std::string & func, const json::w::Document & request);
		int Call(const std::string & func, const json::w::Document & request);
		int Call(const std::string & func, std::unique_ptr<json::r::Document> & response);
		int Call(const std::string & func, const std::string & request, std::unique_ptr<json::r::Document> & response);
		int Call(const std::string & func, const json::w::Document & request, std::unique_ptr<json::r::Document> & response);
	public:
		int LuaSend(lua_State * lua, std::unique_ptr<rpc::Message> &) const;
		int LuaCall(lua_State * lua, std::unique_ptr<rpc::Message> &) const;
		int MakeMessage(lua_State * lua, int idx, const std::string & func, std::unique_ptr<rpc::Message> &) const;
	 public:
		virtual bool OnInit() = 0;
	protected:
		virtual bool GetAddress(const rpc::Message & request, int & id) const = 0;
		virtual std::unique_ptr<rpc::Message> Make(const std::string & func) const = 0;
	protected:
		class RouterComponent * mRouter;
	private:
		const std::string mName;
		class ProtoComponent * mProto;
		std::unordered_map<std::string, std::string> mAddress;
	};

	template<typename T>
	inline int Actor::Call(const std::string& func, std::unique_ptr<T>& response)
	{
		return this->Call(func, response.get());
	}

	template<typename T>
	int Actor::Call(const std::string& func, const pb::Message& request, std::unique_ptr<T>& response)
	{
		return this->Call(func, request, response.get());
	}
}

#endif //APP_ACTOR_H
