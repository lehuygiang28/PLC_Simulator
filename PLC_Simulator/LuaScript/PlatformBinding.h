/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef PLATFORMBINDING_H
#define PLATFORMBINDING_H

#include "ILuaBinding.h"

class IRegisterAccess;
class IPlatformController;
struct lua_State;

// 平台控制绑定：注册 MoveAbsInt32/MoveAbsFloat/MoveRelativeInt32/MoveRelativeFloat/
// WriteCurrentPosInt32/WriteCurrentPosFloat 共 6 个 Lua 函数。
class PlatformBinding : public ILuaBinding {
public:
    PlatformBinding(IRegisterAccess* access, IPlatformController* controller);
    void setController(IPlatformController* controller) { m_controller = controller; }

    void install(lua_State* L) override;
    QList<LuaFunctionDoc> functions() const override;

private:
    IRegisterAccess* m_access;
    IPlatformController* m_controller;

    // 单一来源:install(取 fn)/functions(取 name/snippet/description) 均由此表派生
    struct Fn { const char* name; int (*fn)(lua_State*); const char* snippet; const char* description; };
    static const Fn kFns[];

    // 解析三个地址参数(X/Y/Angle)；失败时已 luaL_error 不返回
    static bool parseThreeAddr(lua_State* L, int& nX, int& nY, int& nA);

    static int MoveAbsInt32Wrapper(lua_State* L);
    static int MoveAbsFloatWrapper(lua_State* L);
    static int MoveRelativeInt32Wrapper(lua_State* L);
    static int MoveRelativeFloatWrapper(lua_State* L);
    static int WriteCurrentPosInt32Wrapper(lua_State* L);
    static int WriteCurrentPosFloatWrapper(lua_State* L);
};

#endif // PLATFORMBINDING_H
