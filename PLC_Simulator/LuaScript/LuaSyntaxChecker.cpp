/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "LuaSyntaxChecker.h"
#include "ILuaBinding.h"
#include "Lua.hpp"

LuaSyntaxChecker::LuaSyntaxChecker()
    : m_state(luaL_newstate())
{
    if (m_state) {
        luaL_openlibs(m_state);
        installEngineStubs();
    }
}

LuaSyntaxChecker::~LuaSyntaxChecker()
{
    if (m_state) { lua_close(m_state); m_state = nullptr; }
}

static int StubVoid(lua_State*) { return 0; }

void LuaSyntaxChecker::installEngineStubs()
{
    // 引擎内建函数的桩；IsLoopValid 返回 nil(假)，与运行期 while 循环不进入一致
    lua_pushcfunction(m_state, StubVoid); lua_setglobal(m_state, "IsLoopValid");
    lua_pushcfunction(m_state, StubVoid); lua_setglobal(m_state, "sleep");
}

void LuaSyntaxChecker::addBinding(ILuaBinding* binding)
{
    if (m_state && binding) binding->installStubs(m_state);
}

bool LuaSyntaxChecker::check(const QString& script, QString& errorMsg)
{
    if (m_state == nullptr) return false;
    QByteArray data = script.toLatin1();
    if (luaL_dostring(m_state, data.data()) != LUA_OK) {
        const char* msg = lua_tostring(m_state, -1);
        lua_pop(m_state, 1);
        errorMsg = QString::fromUtf8(msg);
        return false;
    }
    return true;
}
