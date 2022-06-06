//
// Created by mac on 2022/6/1.
//

#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H
#include"Script/LuaInclude.h"
#include"Component/Scene/MessageComponent.h"
namespace Sentry
{
	class MessageDecoder //转lua
	{
	 public:
		MessageDecoder(lua_State * lua, MessageComponent * component);
		bool Decode(const Message & message);
	 private:
		bool DecodeField(const Message & message, const FieldDescriptor * field);
		bool DecodeSingle(const Message & message, const FieldDescriptor * field);
		bool DecodeTable(const Message & message, const FieldDescriptor * field);
		bool DecodeRepeted(const Message & message, const FieldDescriptor * field);
		bool DecodeRequired(const Message & message, const FieldDescriptor * field);
		bool DecodeOptional(const Message & message, const FieldDescriptor * field);
		bool DecodeMutiple(const Message & message, const FieldDescriptor * field, int index);
	 private:
		lua_State * mLua;
		MessageComponent * mMsgComponent;
	};
}

namespace Sentry
{
	class MessageEncoder
	{
	 public:
		MessageEncoder(lua_State * lua, MessageComponent * component);
	 public:
		std::shared_ptr<Message> Encode(const std::string & proto, int index);
	 private:
		bool EncodeField(Message & message, const FieldDescriptor * field, int index);
		bool EncodeSingle(Message & message, const FieldDescriptor * field, int index);
		bool EncodeTable(Message & message, const FieldDescriptor * field, int index);
		bool EncodeRepeted(Message & message, const FieldDescriptor * field, int index);
		bool EncodeRequired(Message & message, const FieldDescriptor * field, int index);
		bool EncodeOptional(Message & message, const FieldDescriptor * field, int index);
		bool EncodeMutiple(Message & message, const FieldDescriptor * field, int index);
		bool EncoddeMessage(Message & message, const Descriptor * descriptor, int index);
	 private:
		lua_State * mLua;
		MessageComponent * mMsgComponent;
	};
}

namespace Lua
{
	namespace MessageEx
	{
		extern int New(lua_State * lua);
		extern int NewJson(lua_State * lua);
		extern int Decode(lua_State * lua);
		extern int Encode(lua_State * lua);
	};
}


#endif //SERVER_MESSAGE_H