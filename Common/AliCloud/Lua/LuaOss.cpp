//
// Created by yy on 2024/10/9.
//

#include "LuaOss.h"
#include "Entity/Actor/App.h"
#include "Yyjson/Lua/ljson.h"
#include "Lua/Engine/Table.h"
#include "AliCloud/Task/LuaOssTask.h"
#include "Http/Common/HttpResponse.h"
#include "Http/Component/HttpComponent.h"
#include "AliCloud/Component/AliOssComponent.h"


int oss::Sign(lua_State* L)
{
	static std::string bucket;
	static oss::Policy ossPolicy;
	luaL_checktype(L, -1, LUA_TTABLE);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	{
		bucket.clear();
		Lua::Table luaTable(L, ref);
		ossPolicy.limit_type.clear();
		luaTable.Get("bucket", bucket);
		luaTable.Get("file_name", ossPolicy.file_name);
		luaTable.Get("file_type", ossPolicy.file_type);
		luaTable.Get("max_length", ossPolicy.max_length);
		luaTable.Get("expiration", ossPolicy.expiration);
		luaTable.Get("upload_dir", ossPolicy.upload_dir);
		luaTable.Get("limit_type", ossPolicy.limit_type);
	}

	acs::AliOssComponent* ossComponent = acs::App::Get<acs::AliOssComponent>();
	{
		wrap::string<true> json;
		if (!lua::yyjson::read(L, 1, json))
		{
			return false;
		}
		json::r::Document document;
		if (!document.Decode(json.c_str(), json.size(), YYJSON_READ_INSITU))
		{
			return 0;
		}
		oss::FromData ossFromData;
		if (bucket.empty())
		{
			ossComponent->Sign(ossPolicy, ossFromData);
		}
		else
		{
			ossComponent->Sign(ossPolicy, bucket, ossFromData);
		}
		lua_newtable(L);
		Lua::push_value(L, "policy", ossFromData.policy);
		Lua::push_value(L, "OSSAccessKeyId", ossFromData.OSSAccessKeyId);
		Lua::push_value(L, "signature", ossFromData.signature);
		Lua::push_value(L, "key", ossFromData.objectKey);
		Lua::push_value(L, "url", ossFromData.url);
	}
	return 1;
}

int oss::Upload(lua_State* L)
{
	static std::string path;
	static std::string bucket;
	static std::string objectKey;
	bucket.clear();
	if(lua_isstring(L, 1))
	{
		size_t count1 = 0;
		const char* str1 = luaL_checklstring(L, 1, &count1);
		bucket.assign(str1, count1);
	}
	size_t count2 = 0, count3 = 0;
	const char* str2 = luaL_checklstring(L, 2, &count2);
	const char* str3 = luaL_checklstring(L, 3, &count3);

	path.assign(str2, count2);
	objectKey.assign(str3, count3);
	acs::HttpComponent* httpComponent = acs::App::Get<acs::HttpComponent>();
	acs::AliOssComponent* ossComponent = acs::App::Get<acs::AliOssComponent>();
	std::unique_ptr<http::Request> request = bucket.empty() ?
			ossComponent->New(path, objectKey) : ossComponent->New(bucket, path, objectKey);
	std::unique_ptr<http::Response> response1 = std::make_unique<http::Response>();
	if (request == nullptr)
	{
		LOG_ERROR("create request fail");
		return 0;
	}
	int rpcId = 0;
	lua_pushthread(L);
	std::string url = request->GetUrl().ToStr();
	httpComponent->Send(request, response1, rpcId);
	return httpComponent->AddTask(new acs::LuaOssRequestTask(rpcId, L, url))->Await();
}
