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

#ifndef __LLDEBUG_CONTEXT_H__
#define __LLDEBUG_CONTEXT_H__

#include "sysinfo.h"
#include "luainfo.h"
#include "net/remotecommand.h"

namespace lldebug {

class MainFrame;

enum TableType {
	TABLETYPE_GLOBAL,
	TABLETYPE_REGISTRY,
};

class Context {
public:
	enum State {
		STATE_INITIAL,
		STATE_NORMAL,
		STATE_STEPOVER,
		STATE_STEPINTO,
		STATE_STEPRETURN,
		STATE_BREAK,
		STATE_QUIT,
	};

public:
	static Context *Create();
	virtual void Delete();

	static Context *Find(lua_State *L);
	virtual void Quit();
	virtual void SetDebugEnable(bool enabled);

	virtual bool IsDebugEnabled() {
		return m_isEnabled;
	}

	/// ��������E�B���h�E�ɏo�͂��܂��B
	std::string ParseLuaError(const std::string &cstr, std::string *key_,
							  int *line_, bool *isDummyFunc_);
	void OutputLuaError(const char *str);
	void OutputError(const std::string &str);
	void OutputLog(const std::string &str);
	void OutputLog(LogType type, const std::string &str);

	int LoadFile(const char *filename);
	int LoadString(const char *str);

	int LuaOpenBase(lua_State *L);
	void LuaOpenLibs(lua_State *L);

	LuaVarList LuaGetFields(TableType type);
	LuaVarList LuaGetFields(const LuaVar &var);
	LuaVarList LuaGetLocals(const LuaStackFrame &stackFrame);
	LuaVarList LuaGetEnviron(const LuaStackFrame &stackFrame);
	LuaVarList LuaGetEvalVarList(const string_array &array, const LuaStackFrame &stackFrame);
	std::string LuaGetEval(const std::string &str, const LuaStackFrame &stackFrame);
	LuaStackList LuaGetStack();
	LuaBacktraceList LuaGetBacktrace();

	int LuaEval(lua_State *L, int level, const std::string &str);

	/// �R���e�L�X�g�̂h�c���擾���܂��B
	int GetId() const {
		return m_id;
	}

	/// lua�I�u�W�F�N�g���擾���܂��B
	lua_State *GetLua() {
		return m_coroutines.back().L;
	}

	/// ��ԍŏ��ɍ쐬���ꂽlua�I�u�W�F�N�g���擾���܂��B
	lua_State *GetMainLua() {
		return m_lua;
	}

	/// Get the source contents.
	const Source *GetSource(const std::string &key) {
		scoped_lock lock(m_mutex);
		return m_sourceManager.Get(key);
	}

	/// Save the source.
	int SaveSource(const std::string &key, const string_array &source) {
		scoped_lock lock(m_mutex);
		return m_sourceManager.Save(key, source);
	}

	/// Find the breakpoint.
	Breakpoint FindBreakpoint(const std::string &key, int line) {
		scoped_lock lock(m_mutex);
		return m_breakpoints.Find(key, line);
	}

	/// Find the next breakpoint.
	Breakpoint NextBreakpoint(const Breakpoint &bp) {
		scoped_lock lock(m_mutex);
		return m_breakpoints.Next(bp);
	}

	/// Set the breakpoint.
	void SetBreakpoint(const Breakpoint &bp) {
		scoped_lock lock(m_mutex);
		m_breakpoints.Set(bp);
	}

	/// Toggle on/off of the breakpoint.
	void ToggleBreakpoint(const std::string &key, int line) {
		scoped_lock lock(m_mutex);
		m_breakpoints.Toggle(key, line);
	}

	/// Toggle on/off of the breakpoint.
	void ChangedBreakpointList(const BreakpointList &bps) {
		scoped_lock lock(m_mutex);
		m_breakpoints = bps;
	}

private:
	explicit Context();
	virtual ~Context();
	virtual int Initialize();
	virtual int CreateDebuggerFrame();
	virtual int LoadConfig();
	virtual int SaveConfig();
	virtual void SetState(State state);
	virtual int HandleCommand();

	static void SetHook(lua_State *L);
	virtual void HookCallback(lua_State *L, lua_Debug *ar);
	static void s_HookCallback(lua_State *L, lua_Debug *ar);

	class LuaImpl;
	friend class LuaImpl;
	int LuaInitialize(lua_State *L);
	void BeginCoroutine(lua_State *L);
	void EndCoroutine(lua_State *L);

private:
	int LuaIndexForEval(lua_State *L);
	int LuaNewindexForEval(lua_State *L);

private:
	class ContextManager;
	static shared_ptr<ContextManager> ms_manager;
	static int ms_idCounter;

	mutex m_mutex;
	int m_id;
	lua_State *m_lua;
	State m_state;
	bool m_isEnabled;
	int m_waitUpdateCount;
	int m_updateCount;
	bool m_isMustUpdate;

	/// lua_State *���Ƃ̊֐��Ăяo���񐔂��L�^���邱�Ƃ�
	/// �X�e�b�v�I�[�o�[�����S�Ɏ������܂��B
	struct CoroutineInfo {
		CoroutineInfo(lua_State *L_ = NULL, int call_ = 0)
			: L(L_), call(call_) {
		}
		lua_State *L;
		int call;
	};
	typedef std::vector<CoroutineInfo> CoroutineList;
	CoroutineList m_coroutines;
	CoroutineInfo m_stepinfo;

	shared_ptr<RemoteEngine> m_engine;
	SourceManager m_sourceManager;
	BreakpointList m_breakpoints;
	std::string m_rootFileKey;
};

/**
 * @brief ����̃X�R�[�v��lua���g�����߂̃N���X�ł��B
 */
class scoped_lua {
public:
	explicit scoped_lua(lua_State *L)
		: m_L(L), m_top(-1), m_n(-1), m_npop(0) {
		m_ctx = Context::Find(L);
		if (m_ctx != NULL) {
			m_isOldEnabled = m_ctx->IsDebugEnabled();
			m_ctx->SetDebugEnable(false);
		}
	}

	explicit scoped_lua(lua_State *L, int n, int npop = 0)
		: m_L(L), m_n(n), m_npop(npop) {
		m_ctx = Context::Find(L);
		if (m_ctx != NULL) {
			m_isOldEnabled = m_ctx->IsDebugEnabled();
			m_ctx->SetDebugEnable(false);
		}
		m_top = lua_gettop(L);
	}

	~scoped_lua() {
		if (m_top >= 0) {
			assert(m_top + m_n == lua_gettop(m_L));
		}
		lua_pop(m_L, m_npop);

		if (m_ctx != NULL) {
			m_ctx->SetDebugEnable(m_isOldEnabled);
		}
	}

	void reset_stackn(int n) {
		m_n = n;
	}

private:
	Context *m_ctx;
	lua_State *m_L;
	int m_top, m_n;
	int m_npop;
	bool m_isOldEnabled;
};

}

#endif
