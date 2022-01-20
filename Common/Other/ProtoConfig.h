﻿#pragma once

#include <string>

namespace Sentry
{
    class ProtoConfig
    {
    public:
        int Timeout;
        bool IsAsync;
        int MethodId;
        std::string Method;
        std::string Service;
        std::string Request;
        std::string Response;
    };
}// namespace Sentry