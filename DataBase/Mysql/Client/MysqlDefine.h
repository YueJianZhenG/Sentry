#pragma once
#ifdef __ENABLE_MYSQL__
#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#endif

#include<vector>
#include<sstream>
#include"mysql.h"
#include<memory>
#include<unordered_map>
#include"XCode/XCode.h"

using namespace std;
namespace Tendo
{
    class SqlHelper;

   // typedef MYSQL_RES MysqlQueryResult;
    //typedef MYSQL MysqlSocket;
}
#endif