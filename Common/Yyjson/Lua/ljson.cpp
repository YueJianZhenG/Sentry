#define LUA_LIB

#include "ljson.h"
#ifdef __ENABLE_MI_MALLOC__
#include "mimalloc.h"
#endif
#include "Lua/Engine/UserDataParameter.h"
namespace lua
{
	JsonValue::~JsonValue()
	{
		if(this->mDelete)
		{
			yyjson_mut_doc_free(this->doc);
		}
	}

	int yyjson::read_file(lua_State* L)
	{
		const char* path = luaL_checkstring(L, 1);

		yyjson_read_err readErr;
		yyjson_doc* doc = yyjson_read_file(path, YYJSON_READ_ALLOW_INVALID_UNICODE, nullptr, &readErr);
		if (doc == nullptr)
		{
			return 0;
		}
		size_t size = 0;
		yyjson_write_flag flag = YYJSON_WRITE_ALLOW_INVALID_UNICODE;
		char* str = yyjson_write(doc, flag, &size);
		if (str != nullptr)
		{
			yyjson::write(L, str, size);
		}
#ifndef __ENABLE_MI_MALLOC__
		free(str);
#else
		mi_free(str);
#endif
		yyjson_doc_free(doc);
		return 1;
	}

	int yyjson::encode(lua_State* L)
	{
		bool empty_as_array = luaL_opt(L, lua_toboolean, 2, false);
		return encode_impl(L, YYJSON_WRITE_ALLOW_INVALID_UNICODE, empty_as_array);
	}

	int yyjson::pretty(lua_State* L)
	{
		bool empty_as_array = luaL_opt(L, lua_toboolean, 2, false);
		yyjson_write_flag flag = YYJSON_WRITE_ALLOW_INVALID_UNICODE | YYJSON_WRITE_PRETTY;
		return encode_impl(L, flag, empty_as_array);
	}

	int yyjson::encode_impl(lua_State* L, yyjson_write_flag flag, bool emy_as_arr)
	{
		size_t data_len;
		yyjson_write_err err;
		yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
		yyjson_mut_val* val = encode_one(L, doc, emy_as_arr, 1, 0);
		char* json = yyjson_mut_val_write_opts(val, flag, nullptr, &data_len, &err);
		if(json == nullptr)
		{
			lua_pushnil(L);
			yyjson_mut_doc_free(doc);
			return 0;
		}
		lua_pushlstring(L, json, data_len);
#ifndef __ENABLE_MI_MALLOC__
		free(json);
#else
		mi_free(json);
#endif
		return 1;
	}

	bool yyjson::read(lua_State* L, int idx, lua::JsonValue& jsonValue)
	{
		jsonValue.doc = yyjson_mut_doc_new(nullptr);
		jsonValue.val = encode_one(L, jsonValue.doc, false, idx, 0);
		return true;
	}

	bool yyjson::read(lua_State* L, int idx, std::string& data)
	{
		size_t data_len;
		yyjson_write_err err;
		yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
		yyjson_mut_val* val = encode_one(L, doc, false, idx, 0);
		yyjson_write_flag flag = YYJSON_WRITE_ALLOW_INVALID_UNICODE;
		char* json = yyjson_mut_val_write_opts(val, flag, nullptr, &data_len, &err);
		if (json == nullptr || data_len == 0)
		{
			yyjson_mut_doc_free(doc);
			return false;
		}
		data.assign(json, data_len);
		yyjson_mut_doc_free(doc);
#ifndef __ENABLE_MI_MALLOC__
		free(json);
#else
		mi_free(json);
#endif
		return true;
	}

	bool yyjson::read(lua_State* L, int idx, wrap::string<true>& json)
	{
		size_t count = 0;
		yyjson_write_err err;
		yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
		yyjson_mut_val* val = encode_one(L, doc, false, idx, 0);
		yyjson_write_flag flag = YYJSON_WRITE_ALLOW_INVALID_UNICODE;
		char* str = yyjson_mut_val_write_opts(val, flag, nullptr, &count, &err);
		if (str == nullptr || count == 0)
		{
			yyjson_mut_doc_free(doc);
			return false;
		}
		json.assign(str, count);
		yyjson_mut_doc_free(doc);
		return true;
	}
}

namespace lua
{
	int yyjson::decode(lua_State* L)
	{
		size_t len;
		const char* buf = luaL_checklstring(L, 1, &len);
		return write(L, buf, len);
	}

	bool yyjson::write(lua_State* L, const char* buf, size_t len)
	{
		bool result = true;
		yyjson_read_err err;
		yyjson_read_flag flag = YYJSON_READ_INSITU | YYJSON_READ_ALLOW_INVALID_UNICODE;
		yyjson_doc* doc = yyjson_read_opts((char*)buf, len, flag, nullptr, &err);
		if (!doc)
		{
			result = false;
		}

		decode_one(L, yyjson_doc_get_root(doc));
		yyjson_doc_free(doc);
		return result;
	}

//	bool yyjson::write(lua_State* L, yyjson_doc* doc, bool numkeyable)
//	{
//		decode_one(L, yyjson_doc_get_root(doc), numkeyable);
//		return true;
//	}

	bool yyjson::write(lua_State* L, yyjson_val* val)
	{
		decode_one(L, val);
		return true;
	}

	bool yyjson::is_array(lua_State* L, int index, bool emy_as_arr)
	{
		size_t raw_len = lua_rawlen(L, index);
		if (raw_len == 0 && !emy_as_arr)
		{
			return false;
		}
		lua_pushnil(L);
		size_t cur_len = 0;
		while (lua_next(L, index) != 0)
		{
			if (!lua_isinteger(L, -2))
			{
				return false;
			}
			size_t key = lua_tointeger(L, -2);
			if (key <= 0 || key > raw_len)
			{
				return false;
			}
			lua_pop(L, 1);
			cur_len++;
		}
		if (cur_len == 0) return true;
		return cur_len == raw_len;
	}

	yyjson_mut_val* yyjson::encode_one(lua_State* L, yyjson_mut_doc* doc, bool emy_as_arr, int idx, int depth)
	{
		if (depth > max_encode_depth)
		{
			return nullptr;
		}
		int type = lua_type(L, idx);
		switch (type)
		{
			case LUA_TNIL:
				return yyjson_mut_null(doc);
			case LUA_TBOOLEAN:
				return yyjson_mut_bool(doc, lua_toboolean(L, idx));
			case LUA_TNUMBER:
				return lua_isinteger(L, idx) ? yyjson_mut_sint(doc, lua_tointeger(L, idx)) : yyjson_mut_real(doc,
						lua_tonumber(L, idx));
			case LUA_TSTRING:
			{
				size_t len;
				const char* val = lua_tolstring(L, idx, &len);
				return yyjson_mut_strn(doc, val, len);
			}
			case LUA_TTABLE:
				return table_encode(L, doc, emy_as_arr, idx, depth + 1);
			case LUA_TUSERDATA:
			case LUA_TLIGHTUSERDATA:
				return yyjson_mut_str(doc, "unsupported userdata");
			case LUA_TFUNCTION:
				return yyjson_mut_str(doc, "unsupported function");
			case LUA_TTHREAD:
				return yyjson_mut_str(doc, "unsupported thread");
		}
		return yyjson_mut_str(doc, "unsupported datatype");
	}

	yyjson_mut_val* yyjson::key_encode(lua_State* L, yyjson_mut_doc* doc, int idx)
	{
		switch (lua_type(L, idx))
		{
			case LUA_TSTRING:
				return yyjson_mut_str(doc, lua_tostring(L, idx));
			case LUA_TNUMBER:
				if (lua_isinteger(L, idx))
				{
					return yyjson_mut_strcpy(doc, std::to_string(lua_tointeger(L, idx)).c_str());
				}
				return yyjson_mut_strcpy(doc, std::to_string(lua_tonumber(L, idx)).c_str());
		}
		return nullptr;
	}

	yyjson_mut_val* yyjson::array_encode(lua_State* L, yyjson_mut_doc* doc, bool emy_as_arr, int index, int depth)
	{
		lua_pushnil(L);
		yyjson_mut_val* array = yyjson_mut_arr(doc);
		while (lua_next(L, index) != 0)
		{
			yyjson_mut_arr_append(array, encode_one(L, doc, emy_as_arr, -1, depth));
			lua_pop(L, 1);
		}
		return array;
	}

	yyjson_mut_val* yyjson::table_encode(lua_State* L, yyjson_mut_doc* doc, bool emy_as_arr, int index, int depth)
	{
		index = lua_absindex(L, index);
		if (!is_array(L, index, emy_as_arr))
		{
			lua_pushnil(L);
			yyjson_mut_val* object = yyjson_mut_obj(doc);
			while (lua_next(L, index) != 0)
			{
				auto key = key_encode(L, doc, -2);
				if (key != nullptr)
				{
					auto value = encode_one(L, doc, emy_as_arr, -1, depth);
					unsafe_yyjson_mut_obj_add(object, key, value, unsafe_yyjson_get_len(object));
				}
				lua_pop(L, 1);
			}
			return object;
		}
		return array_encode(L, doc, emy_as_arr, index, depth);
	}

	void yyjson::array_decode(lua_State* L, yyjson_val* val)
	{
		yyjson_arr_iter it;
		yyjson_arr_iter_init(val, &it);
		lua_createtable(L, 0, (int)yyjson_arr_size(val));
		while (yyjson_val * value = yyjson_arr_iter_next(&it))
		{
			decode_one(L, value);
			lua_rawseti(L, -2, it.idx);
		}
	}

	void yyjson::table_decode(lua_State* L, yyjson_val* val)
	{
		yyjson_obj_iter it;
		yyjson_obj_iter_init(val, &it);
		lua_createtable(L, 0, (int)yyjson_obj_size(val));
		while(yyjson_val * key = yyjson_obj_iter_next(&it))
		{
			const char * k = unsafe_yyjson_get_str(key);
			const size_t count = unsafe_yyjson_get_len(key);
			yyjson_val * value = yyjson_obj_iter_get_val(key);

			lua_pushlstring(L, k, count);
			decode_one(L, value);
			lua_rawset(L, -3);
		}
	}

	void yyjson::number_decode(lua_State* L, yyjson_val* val)
	{
		switch (yyjson_get_subtype(val))
		{
			case YYJSON_SUBTYPE_UINT:
			case YYJSON_SUBTYPE_SINT:
				lua_pushinteger(L, unsafe_yyjson_get_sint(val));
				break;
			case YYJSON_SUBTYPE_REAL:
				lua_pushnumber(L, unsafe_yyjson_get_real(val));
				break;
		}
	}


	void yyjson::decode_one(lua_State* L, yyjson_val* val)
	{
		switch (yyjson_get_type(val))
		{
		case YYJSON_TYPE_NULL:
		case YYJSON_TYPE_NONE:
			lua_pushnil(L);
			break;
		case YYJSON_TYPE_BOOL:
			lua_pushboolean(L, unsafe_yyjson_get_bool(val));
			break;
		case YYJSON_TYPE_STR:
			lua_pushlstring(L, unsafe_yyjson_get_str(val), unsafe_yyjson_get_len(val));
			break;
		case YYJSON_TYPE_NUM:
			number_decode(L, val);
			break;
		case YYJSON_TYPE_ARR:
			array_decode(L, val);
			break;
		case YYJSON_TYPE_OBJ:
			table_decode(L, val);
			break;
		case YYJSON_TYPE_RAW:
			{
				size_t size = unsafe_yyjson_get_len(val);
				const char * str = unsafe_yyjson_get_raw(val);
				if(str != nullptr && size > 0)
				{
					lua_pushlstring(L, str, size);
				}
				break;
			}
		default:
			lua_pushnil(L);
			break;
		}
	}


	int ljson::create(lua_State* L)
	{
		bool isArray = false;
		if (lua_isboolean(L, 1))
		{
			isArray = lua_toboolean(L, 1);
		}
		std::unique_ptr<lua::JsonValue> jsonValue = std::make_unique<lua::JsonValue>(true);
		{
			jsonValue->doc = yyjson_mut_doc_new(nullptr);
			jsonValue->val = isArray ? yyjson_mut_arr(jsonValue->doc)
									 : yyjson_mut_obj(jsonValue->doc);
			yyjson_mut_doc_set_root(jsonValue->doc, jsonValue->val);
		}
		Lua::UserDataParameter::UserDataStruct<lua::JsonValue*>::WritePtr(L, jsonValue.release());
		return 1;
	}

	int ljson::add_array(lua_State* L)
	{
		lua::JsonValue* jsonValue = Lua::UserDataParameter::Read<lua::JsonValue*>(L, 1);
		if (jsonValue == nullptr)
		{
			luaL_error(L, "json value object is null");
			return 0;
		}
		std::unique_ptr<lua::JsonValue> newJsonArray = std::make_unique<lua::JsonValue>(false);
		if (yyjson_mut_is_arr(jsonValue->val))
		{
			newJsonArray->val = yyjson_mut_arr(jsonValue->doc);
			if (!yyjson_mut_arr_add_val(jsonValue->val, newJsonArray->val))
			{
				return 0;
			}
		}
		else if(lua_isstring(L, 2))
		{
			size_t count = 0;
			const char* str = luaL_tolstring(L, 2, &count);
			newJsonArray->val = yyjson_mut_arr(jsonValue->doc);
			yyjson_mut_val* key = yyjson_mut_strn(jsonValue->doc, str, count);
			if (!yyjson_mut_obj_add(jsonValue->val, key, newJsonArray->val))
			{
				return 0;
			}
		}
		newJsonArray->doc = jsonValue->doc;
		Lua::UserDataParameter::UserDataStruct<lua::JsonValue *>::WritePtr(L, newJsonArray.release());
		return 1;
	}

	int ljson::add_object(lua_State* L)
	{
		lua::JsonValue* jsonValue = Lua::UserDataParameter::Read<lua::JsonValue*>(L, 1);
		if (jsonValue == nullptr)
		{
			luaL_error(L, "json value object is null");
			return 0;
		}
		std::unique_ptr<lua::JsonValue> newJsonArray = std::make_unique<lua::JsonValue>(false);
		if (yyjson_mut_is_arr(jsonValue->val))
		{
			newJsonArray->val = yyjson_mut_obj(jsonValue->doc);
			if (!yyjson_mut_arr_add_val(jsonValue->val, newJsonArray->val))
			{
				return 0;
			}
		}
		else if(lua_isstring(L, 2))
		{
			size_t count = 0;
			const char* str = luaL_tolstring(L, 2, &count);
			newJsonArray->val = yyjson_mut_obj(jsonValue->doc);
			yyjson_mut_val* key = yyjson_mut_strn(jsonValue->doc, str, count);
			if (!yyjson_mut_obj_add(jsonValue->val, key, newJsonArray->val))
			{
				return 0;
			}
		}
		newJsonArray->doc = jsonValue->doc;
		Lua::UserDataParameter::UserDataStruct<lua::JsonValue*>::WritePtr(L, newJsonArray.release());
		return 1;
	}

	yyjson_mut_val* ljson::encode(lua_State* L, yyjson_mut_doc* doc, int index)
	{
		switch (lua_type(L, index))
		{
			case LUA_TNIL:
				return yyjson_mut_null(doc);
			case LUA_TNUMBER:
			{
				if (lua_isinteger(L, index))
				{
					long long number = lua_tointeger(L, index);
					return yyjson_mut_int(doc, number);
				}
				double number = lua_tonumber(L, index);
				return yyjson_mut_real(doc, number);
			}
			case LUA_TBOOLEAN:
			{
				bool item = lua_toboolean(L, index);
				return yyjson_mut_bool(doc, item);
			}
			case LUA_TSTRING:
			{
				size_t len = 0;
				const char* str = luaL_tolstring(L, index, &len);
				return yyjson_mut_strn(doc, str, len);
			}
		}
		return nullptr;
	}

	int ljson::add_member(lua_State* L)
	{
		lua::JsonValue* jsonValue = Lua::UserDataParameter::Read<lua::JsonValue*>(L, 1);
		if (jsonValue == nullptr)
		{
			luaL_error(L, "json value object is null");
			return 0;
		}
		if (yyjson_mut_is_arr(jsonValue->val))
		{
			yyjson_mut_val* value = ljson::encode(L, jsonValue->doc, 2);
			if(value == nullptr)
			{
				luaL_error(L, "must number or string or bool");
				return 0;
			}
			lua_pushboolean(L, yyjson_mut_arr_add_val(jsonValue->val, value));
			return 1;
		}
		if(lua_isstring(L, 2))
		{
			size_t count = 0;
			const char * str = luaL_tolstring(L, 2, &count);
			yyjson_mut_val* key = yyjson_mut_strn(jsonValue->doc, str, count);
			yyjson_mut_val* value = ljson::encode(L, jsonValue->doc, 3);
			lua_pushboolean(L, yyjson_mut_obj_add(jsonValue->val, key, value));
			return 1;
		}

		return 0;
	}

	int ljson::encode(lua_State* L)
	{
		lua::JsonValue* jsonValue = Lua::UserDataParameter::Read<lua::JsonValue*>(L, 1);
		if (jsonValue == nullptr)
		{
			luaL_error(L, "json value object is null");
			return 0;
		}
		yyjson_write_flag flag = YYJSON_WRITE_ALLOW_INVALID_UNICODE;
		if(lua_isboolean(L, 2) && lua_toboolean(L, 2))
		{
			flag = flag | YYJSON_WRITE_PRETTY;
		}
		size_t data_len = 0;
		char* str = yyjson_mut_write(jsonValue->doc, flag, &data_len);
		if(str != nullptr)
		{
			lua_pushlstring(L, str, data_len);
		}
#ifndef __ENABLE_MI_MALLOC__
		free(str);
#else
		mi_free(str);
#endif
		return 1;
	}
}