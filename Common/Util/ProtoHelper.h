//
// Created by zmhy0073 on 2022/8/29.
//

#ifndef APP_PROTOHELPER_H
#define APP_PROTOHELPER_H

#include"google/protobuf/message.h"
using namespace google::protobuf;
namespace Helper
{
    namespace Protocol
    {
        bool GetJson(const Message & message, std::string * json);
        bool GetMember(const char * key, const Message & message, std::string & value);
    }
};


#endif //APP_PROTOHELPER_H
