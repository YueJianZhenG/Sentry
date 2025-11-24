/**
*  $Id: md5.h,v 1.2 2006/03/03 15:04:49 tomas Exp $
*  Cryptographic module for Lua.
*  @author  Roberto Ierusalimschy
*/


#ifndef md5_h
#define md5_h

#include "lua.h"
#include <string>

#define HASHSIZE       16

void md5 (const char *message, long len, char *output);
bool file_md5 (const std::string & path, std::string &output);
int luaopen_md5_core (lua_State *L);


#endif
