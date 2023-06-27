#pragma once
#include<list>
#include<vector>
#include"Entity/Component/Component.h"
#include"Proto/Message/ProtoMessage.h"
#include"Telnet/Client/TelnetClientContext.h"

namespace Tendo
{
	class TelnetProto : public Tcp::ProtoMessage
	{
	 public:
		TelnetProto() = default;
		~TelnetProto() = default;
	 public:
		void Add(const std::string & content);
		void Add(const char * str, size_t size);
		int Serialize(std::ostream & os) final; //返回剩余要发送的字节数
	 private:
		std::list<std::string> mContents;
	};
}

namespace Tcp
{
	class TelnetClientContext;
}
namespace Tendo
{
	class ConsoleComponent : public Component, public IComplete
	{
	 public:
		ConsoleComponent() = default;
	 private:
		void Close(const std::string & request);
	 private:
		void Update();
	 private:
		class Entity * mCommandUnit;
		class HttpComponent * mHttpComponent;
		class CoroutineComponent* mTaskComponent;
	};
}

