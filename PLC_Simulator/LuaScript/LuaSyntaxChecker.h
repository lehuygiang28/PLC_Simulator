/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef LUASYNTAXCHECKER_H
#define LUASYNTAXCHECKER_H

#include <QString>
#include <QList>

class ILuaBinding;
struct lua_State;

// 进程级 Lua 语法检查器：装入各绑定的桩 + 引擎内建桩(IsLoopValid/sleep)，
// 用 luaL_dostring 跑一遍以校验语法。注意:仅语法级，未定义函数若在未执行路径仍漏检(Lua 特性)。
class LuaSyntaxChecker {
public:
    LuaSyntaxChecker();
    ~LuaSyntaxChecker();

    // 注册绑定的桩(须在 check 前调用)
    void addBinding(ILuaBinding* binding);

    // 返回 true 表示语法通过；否则 errorMsg 含错误信息
    bool check(const QString& script, QString& errorMsg);

    // 暴露校验状态机,供 Host 装入内建/模块桩
    lua_State* state() const { return m_state; }

private:
    lua_State* m_state;
    void installEngineStubs();  // IsLoopValid / sleep 桩
};

#endif // LUASYNTAXCHECKER_H
