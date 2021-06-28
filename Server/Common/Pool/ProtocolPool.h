﻿#pragma once
#include<queue>
#include<unordered_map>
#include<google/protobuf/message.h>
using namespace google::protobuf;
#define ProtocolMaxCount 10
namespace SoEasy
{
	class ProtocolPool
	{
	public:
		static ProtocolPool * Get();
		Message * Create(const std::string & name);
		bool Destory(Message * messageData);
	private:
		std::unordered_map<std::string, std::queue<Message *>> mProtocolMap;
	};
}