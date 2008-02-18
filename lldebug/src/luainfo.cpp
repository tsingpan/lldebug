/*
 * Copyright (c) 2005-2008  cielacanth <cielacanth AT s60.xrea.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "precomp.h"
#include "luainfo.h"

namespace lldebug {

// #define LUA_TNONE		(-1)
// #define LUA_TNIL		0
// #define LUA_TBOOLEAN		1
// #define LUA_TLIGHTUSERDATA	2
// #define LUA_TNUMBER		3
// #define LUA_TSTRING		4
// #define LUA_TTABLE		5
// #define LUA_TFUNCTION		6
// #define LUA_TUSERDATA		7
// #define LUA_TTHREAD		8
std::string LuaGetTypeName(int type) {
	const static std::string s_typenames[] = {
		"nil",
		"boolean",
		"lightuserdata",
		"number",
		"string",
		"table",
		"function",
		"userdata",
		"thread",
	};
	const static int size =
		(sizeof(s_typenames) / sizeof(s_typenames[0]));
	
	if (type < 0 || size <= type) {
		return std::string("");
	}

	return s_typenames[type];
}

#ifdef LLDEBUG_CONTEXT
const int LuaOriginalObject = 0;

std::string LuaToString(lua_State *L, int idx) {
	int type = lua_type(L, idx);
	std::string str;
	char buffer[512];

	switch (type) {
	case LUA_TNONE:
		str = "none";
		break;
	case LUA_TNIL:
		str = "nil";
		break;
	case LUA_TBOOLEAN:
		str = (lua_toboolean(L, idx) ? "true" : "false");
		break;
	case LUA_TNUMBER: {
		lua_Number d = lua_tonumber(L, idx);
		lua_Integer i;
		lua_number2int(i, d);
		if ((lua_Number)i == d) {
			snprintf(buffer, sizeof(buffer), "%d", i);
		}
		else {
			snprintf(buffer, sizeof(buffer), "%f", d);
		}
		str = buffer;
		}
		break;
	case LUA_TSTRING:
		str = lua_tostring(L, idx);
		break;
	case LUA_TFUNCTION:
	case LUA_TTHREAD:
	case LUA_TTABLE:
	case LUA_TUSERDATA:
	case LUA_TLIGHTUSERDATA:
		snprintf(buffer, sizeof(buffer), "%s: %p", lua_typename(L, type), lua_topointer(L, idx));
		str = buffer;
		break;
	default:
		return std::string("");
	}

	return str;
}

std::string LuaConvertString(lua_State *L, int idx) {
	int top = lua_gettop(L);
	const char *cstr;
	std::string str;

	lua_pushliteral(L, "lldebug");
	lua_gettable(L, LUA_GLOBALSINDEX);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		goto on_error;
	}

	lua_pushliteral(L, "tostring");
	lua_gettable(L, -2);
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		goto on_error;
	}
	lua_remove(L, -2); // eliminate 'lldebug'.

	lua_pushvalue(L, idx);
	if (lua_pcall(L, 1, 1, 0) != 0) {
		lua_pop(L, 1); // error string
		goto on_error;
	}

	cstr = lua_tostring(L, -1);
	str = (cstr != NULL ? cstr : "");
	lua_pop(L, 1);

	assert(top == lua_gettop(L));
	return str;

on_error:;
	assert(top == lua_gettop(L));
	return std::string("error on lldebug.tostring");
}

std::string LuaMakeFuncName(lua_Debug *ar) {
	std::string name;

	if (*ar->namewhat != '\0') { /* is there a name? */
		name = ar->name;
	}
	else {
		if (*ar->what == 'm') { /* main? */
			name = "main_chunk";
		}
		else if (*ar->what == 'C' || *ar->what == 't') {
			name = std::string("?");  /* C function or tail call */
		}
		else {
			std::stringstream stream;
			stream << "no name [defined <" << ar->short_src << ":" << ar->linedefined << ">]";
			stream.flush();
			name = stream.str();
		}
	}

	return name;
}
#endif


/*-----------------------------------------------------------------*/
LuaStackFrame::LuaStackFrame(const LuaHandle &lua, int level)
	: m_lua(lua), m_level(level) {
}

LuaStackFrame::~LuaStackFrame() {
}


/*-----------------------------------------------------------------*/
#ifdef LLDEBUG_CONTEXT
LuaVar::LuaVar(const LuaHandle &lua, const std::string &name, int valueIdx)
	: m_lua(lua), m_name(name) {
	lua_State *L = lua.GetState();
	m_value = LuaConvertString(L, valueIdx);
	m_valueType = lua_type(L, valueIdx);
	m_tableIdx = RegisterTable(L, valueIdx);
	m_hasFields = CheckHasFields(L, valueIdx);
}

LuaVar::LuaVar(const LuaHandle &lua, const std::string &name,
			   const std::string &value)
	: m_lua(lua), m_name(name), m_value(value), m_valueType(LUA_TNONE)
	, m_tableIdx(-1), m_hasFields(false) {
}
#endif

LuaVar::LuaVar()
	: m_valueType(-1), m_tableIdx(-1), m_hasFields(false) {
}

LuaVar::~LuaVar() {
}

#ifdef LLDEBUG_CONTEXT
bool LuaVar::CheckHasFields(lua_State *L, int valueIdx) const {
	// Check weather 'valueIdx' has metatable.
	if (lua_getmetatable(L, valueIdx) != 0) {
		lua_pop(L, 1);
		return true;
	}

	if (!lua_istable(L, valueIdx)) {
		return false;
	}

	// Check weather 'valueIdx' has fields.
	lua_pushnil(L);
	if (lua_next(L, valueIdx) != 0) {
		lua_pop(L, 2);
		return true;
	}

	return false;
}

int LuaVar::RegisterTable(lua_State *L, int valueIdx) {
	if (!lua_istable(L, valueIdx)) {
		if (lua_getmetatable(L, valueIdx) == 0) {
			return -1;
		}
		lua_pop(L, 1);
	}

	// OriginalObj couldn't be handle correctly.
	if (lua_islightuserdata(L, valueIdx)
		&& lua_topointer(L, valueIdx) == &LuaOriginalObject) {
		return -1;
	}

	int top = lua_gettop(L);

	// Try to do "table = registry[&OriginalObj]"
	lua_pushlightuserdata(L, (void *)&LuaOriginalObject);
	lua_rawget(L, LUA_REGISTRYINDEX);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);

		// local newtable = {}
		lua_newtable(L);

		// setmetatable(newtable, {__mode="vk"})
		lua_newtable(L);
		lua_pushliteral(L, "__mode");
		lua_pushliteral(L, "vk");
		lua_rawset(L, -3);
		lua_setmetatable(L, -2);

		// newtable[0] = 1 -- initial index
		lua_pushinteger(L, 1);
		lua_rawseti(L, -2, 0);

		// registry[&OriginalObj] = newtable
		lua_pushlightuserdata(L, (void *)&LuaOriginalObject);
		lua_pushvalue(L, -2);
		lua_rawset(L, LUA_REGISTRYINDEX);
	}

	// table is registry[&OriginalObj]
	int table = lua_gettop(L);
	int n = 0; // use after

	// Check table[valueIdx] is number.
	lua_pushvalue(L, valueIdx);
	lua_rawget(L, table);
	if (lua_isnumber(L, -1)) {
		n = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);

		// Check table[n] is table.
		lua_rawgeti(L, table, n);
		if (lua_istable(L, -1)) {
			// No problems.
			lua_pop(L, 1);
		}
		else {
			lua_pop(L, 1);

			// table[n] = valueIdx, again
			lua_pushvalue(L, valueIdx);
			lua_rawseti(L, table, n);
		}
	}
	else {
		lua_pop(L, 1);

		// n = table[0]
		lua_rawgeti(L, table, 0);
		n = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);

		// table[0] = n + 1
		lua_pushinteger(L, n + 1);
		lua_rawseti(L, table, 0);

		// table[valueIdx] = n
		lua_pushvalue(L, valueIdx);
		lua_pushinteger(L, n);
		lua_rawset(L, table);

		// table[n] = valueIdx
		lua_pushvalue(L, valueIdx);
		lua_rawseti(L, table, n);
	}
	
	lua_pop(L, 1);
	assert(top == lua_gettop(L));
	return n;
}

int LuaVar::PushTable(lua_State *L) const {
	if (m_tableIdx < 0) {
		return -1;
	}

	// push registry[&OriginalObj][m_tableIdx]
	lua_pushlightuserdata(L, (void *)&LuaOriginalObject);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_rawgeti(L, -1, m_tableIdx);
	lua_remove(L, -2);
	return 0;
}
#endif


/*-----------------------------------------------------------------*/
#ifdef LLDEBUG_CONTEXT
LuaBacktrace::LuaBacktrace(const LuaHandle &lua,
						   const std::string &name,
						   const std::string &sourceKey,
						   const std::string &sourceTitle,
						   int line, int level)
	: m_lua(lua), m_funcName(name)
	, m_key(sourceKey), m_sourceTitle(sourceTitle)
	, m_line(line), m_level(level) {
}
#endif

LuaBacktrace::LuaBacktrace() {
}

LuaBacktrace::~LuaBacktrace() {
}

}