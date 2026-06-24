/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef REGISTERBINDING_H
#define REGISTERBINDING_H

#include "ILuaBinding.h"

class RegisterStore;
struct lua_State;

// 寄存器读写绑定：注册 SetInt16/GetInt16/... 等 10 个 Lua 函数。
// 无状态(仅持 store 指针)，可被多个 lua_State 共享安装。
class RegisterBinding : public ILuaBinding {
public:
    explicit RegisterBinding(RegisterStore* store);

    void install(lua_State* L) override;
    QList<LuaFunctionDoc> functions() const override;

private:
    RegisterStore* m_store;

    // 单一来源:install(取 fn)/functions(取 name/snippet/description) 均由此表派生
    struct Fn { const char* name; int (*fn)(lua_State*); const char* snippet; const char* description; };
    static const Fn kFns[];

    // Lua C 封装函数(upvalue=RegisterBinding*)
    static int SetInt16Wrapper(lua_State* L);
    static int SetInt32Wrapper(lua_State* L);
    static int SetFloatWrapper(lua_State* L);
    static int SetDoubleWrapper(lua_State* L);
    static int SetStringWrapper(lua_State* L);
    static int GetInt16Wrapper(lua_State* L);
    static int GetInt32Wrapper(lua_State* L);
    static int GetFloatWrapper(lua_State* L);
    static int GetDoubleWrapper(lua_State* L);
    static int GetStringWrapper(lua_State* L);
};

#endif // REGISTERBINDING_H
