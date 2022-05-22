//
// Created by yjz on 2022/5/18.
//

#ifndef _RPCPROTOMESSAGE_H_
#define _RPCPROTOMESSAGE_H_
#include"ProtoMessage.h"
#include"Network/Rpc.h"
using namespace google::protobuf;
namespace Tcp
{
	namespace Rpc
	{
		class RpcProtoMessage final : public Tcp::ProtoMessage
		{
		public:
			RpcProtoMessage(RPC_TYPE type, std::shared_ptr<Message> message);
			bool Serailize(std::ostream &os) final;
		private:
			RPC_TYPE mType;
			std::shared_ptr<Message> mMessage;
		};
	}
}

#endif //_RPCPROTOMESSAGE_H_