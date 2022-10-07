//
// Created by zmhy0073 on 2021/10/14.
//

#ifndef GameKeeper_MESSAGESTREAM_H
#define GameKeeper_MESSAGESTREAM_H
#include<stdexcept>
#include<unordered_map>
#include"Log/CommonLogDef.h"
namespace Tcp
{
    enum class Type
    {
        None,
        Request,
        Response
    };
    enum class Porto
    {
        None,
        Json,
        Protobuf
    };
}


typedef std::logic_error rpc_error;
constexpr int RPC_PACK_HEAD_LEN = sizeof(int) + sizeof(char) + sizeof(char);

#endif //GameKeeper_MESSAGESTREAM_H