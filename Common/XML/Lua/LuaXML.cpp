//
// Created by 64658 on 2025/9/1.
//

#include "LuaXML.h"
#include <vector>
#include "XML/Src/tinyxml2.h"

using namespace tinyxml2;
namespace lua
{
	namespace xml {

		static void push_elem2lua(lua_State* L, const XMLElement* elem) {
			uint32_t count = elem->ChildElementCount();
			const XMLAttribute* attr = elem->FirstAttribute();
			const char* value = elem->GetText();
			if (count == 0 && attr == nullptr) {
				value = value ? value : "";
				if (lua_stringtonumber(L, value) == 0) {
					lua_pushstring(L, value);
				}
				return;
			}
			lua_createtable(L, 0, 4);
			if (value) {
				if (lua_stringtonumber(L, value) == 0) {
					lua_pushstring(L, value);
				}
				lua_seti(L, -2, 1);
			}
			if (attr) {
				lua_createtable(L, 0, 4);
				while (attr) {
					if (lua_stringtonumber(L, attr->Value()) == 0) {
						lua_pushstring(L, attr->Value());
					}
					lua_setfield(L, -2, attr->Name());
					attr = attr->Next();
				}
				lua_setfield(L, -2, "_attr");
			}
			if (count > 0) {
				const XMLElement* child = elem->FirstChildElement();
				std::map<std::string, std::vector<const XMLElement*>> elems;
				while (child) {
					auto it = elems.find(child->Name());
					if (it != elems.end()) {
						it->second.push_back(child);
					} else {
						std::vector<const XMLElement *> item;
						item.emplace_back(child);
						elems.emplace(child->Name(), item);
					}
					child = child->NextSiblingElement();
				}
				for (auto & it : elems) {
					size_t child_size = it.second.size();
					if (child_size == 1) {
						push_elem2lua(L, it.second[0]);
					} else {
						lua_createtable(L, 0, 4);
						for (size_t i = 0; i < child_size; ++i) {
							push_elem2lua(L, it.second[i]);
							lua_seti(L, -2, i + 1);
						}
					}
					lua_setfield(L, -2, it.first.c_str());
				}
			}
		}
		static void load_elem4lua(lua_State* L, XMLPrinter* printer);
		static void load_table4lua(lua_State* L, XMLPrinter* printer) {
			if (lua_getfield(L, -1, "_attr") == LUA_TTABLE) {
				lua_pushnil(L);
				while (lua_next(L, -2) != 0) {
					const char* key = lua_tostring(L, -2);
					switch (lua_type(L, -1)) {
						case LUA_TSTRING: printer->PushAttribute(key, lua_tostring(L, -1)); break;
						case LUA_TBOOLEAN: printer->PushAttribute(key, lua_toboolean(L, -1)); break;
						case LUA_TNUMBER: lua_isinteger(L, -1) ? printer->PushAttribute(key, int64_t(lua_tointeger(L, -1))) : printer->PushAttribute(key, lua_tonumber(L, -1)); break;
					}
					lua_pop(L, 1);
				}
			}
			lua_pushnil(L);
			lua_setfield(L, -3, "_attr");
			switch (lua_geti(L, -2, 1)) {
				case LUA_TSTRING: printer->PushText(lua_tostring(L, -1)); break;
				case LUA_TBOOLEAN: printer->PushText(lua_toboolean(L, -1)); break;
				case LUA_TNUMBER: lua_isinteger(L, -1) ? printer->PushText(int64_t(lua_tointeger(L, -1))) : printer->PushText(lua_tonumber(L, -1)); break;
			}
			lua_pushnil(L);
			lua_seti(L, -4, 1);
			lua_pushnil(L);
			while (lua_next(L, -4) != 0) {
				load_elem4lua(L, printer);
				lua_pop(L, 1);
			}
		}

		inline void load_elem4lua(lua_State* L, XMLPrinter* printer) {
			const char* key = lua_tostring(L, -2);
			if (!Lua::is_lua_array(L, -1)) {
				printer->OpenElement(key);
				switch (lua_type(L, -1)) {
					case LUA_TTABLE: load_table4lua(L, printer); break;
					case LUA_TSTRING: printer->PushText(lua_tostring(L, -1)); break;
					case LUA_TBOOLEAN: printer->PushText(lua_toboolean(L, -1)); break;
					case LUA_TNUMBER: lua_isinteger(L, -1) ? printer->PushText(int64_t(lua_tointeger(L, -1))) : printer->PushText(lua_tonumber(L, -1)); break;
				}
				printer->CloseElement();
				return;
			}
			lua_pushstring(L, key);
			int raw_len = lua_rawlen(L, -2);
			for (int i = 1; i <= raw_len; ++i) {
				lua_rawgeti(L, -2, i);
				load_elem4lua(L, printer);
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
		}

		int write(lua_State* L, const char* xml, size_t size) {
			XMLDocument doc;
			if (doc.Parse(xml, size) != XML_SUCCESS) {
				lua_pushnil(L);
				lua_pushstring(L, "parse xml doc failed!");
				return 2;
			}
			lua_createtable(L, 0, 4);
			const XMLElement* root = doc.RootElement();
			push_elem2lua(L, root);
			lua_setfield(L, -2, root->Name());
			return 1;
		}

		inline int encode_xml(lua_State* L) {
			XMLPrinter printer;
			const char* header = luaL_optstring(L, 2, nullptr);
			if (header) {
				printer.PushDeclaration(header);
			} else {
				printer.PushHeader(false, true);
			}
			lua_pushnil(L);
			while (lua_next(L, 1) != 0) {
				load_elem4lua(L, &printer);
				lua_pop(L, 1);
			}
			lua_pushlstring(L, printer.CStr(), printer.CStrSize());
			return 1;
		}
	}
}

namespace lua
{
	int xml::encode(lua_State* L)
	{
		return xml::encode_xml(L);
	}

	int xml::decode(lua_State* L)
	{
		size_t count = 0;
		const char * str = luaL_checklstring(L, 1, &count);
		return xml::write(L, str, count);
	}
}