/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "LuaSyntaxChecker.h"
#include "Lua.hpp"

LuaSyntaxChecker::LuaSyntaxChecker()
    : m_state(luaL_newstate())
{
    if (m_state) {
        luaL_openlibs(m_state);
    }
}

LuaSyntaxChecker::~LuaSyntaxChecker()
{
    if (m_state) { lua_close(m_state); m_state = nullptr; }
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
