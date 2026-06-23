/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "PlatformBinding.h"
#include "IRegisterAccess.h"
#include "IPlatformController.h"
#include "LuaBindingUtil.h"
#include "Lua.hpp"

PlatformBinding::PlatformBinding(IRegisterAccess* access, IPlatformController* controller)
    : m_access(access), m_controller(controller)
{
}

static PlatformBinding* self_from(lua_State* L)
{
    return static_cast<PlatformBinding*>(lua_touserdata(L, lua_upvalueindex(1)));
}

bool PlatformBinding::parseThreeAddr(lua_State* L, int& nX, int& nY, int& nA)
{
    if (!lua_isstring(L, 1)) { luaL_error(L, "Argument #1 (xReg) must be a string (register address)"); return false; }
    const char* xAddr = lua_tostring(L, 1);
    if (!LuaBindingUtil::parseRegisterAddr(xAddr, nX)) { luaL_error(L, "X register address Invalid: %s", xAddr); return false; }
    if (!lua_isstring(L, 2)) { luaL_error(L, "Argument #2 (yReg) must be a string (register address)"); return false; }
    const char* yAddr = lua_tostring(L, 2);
    if (!LuaBindingUtil::parseRegisterAddr(yAddr, nY)) { luaL_error(L, "Y register address Invalid: %s", yAddr); return false; }
    if (!lua_isstring(L, 3)) { luaL_error(L, "Argument #3 (angleReg) must be a string (register address)"); return false; }
    const char* aAddr = lua_tostring(L, 3);
    if (!LuaBindingUtil::parseRegisterAddr(aAddr, nA)) { luaL_error(L, "Angle register address Invalid: %s", aAddr); return false; }
    return true;
}

void PlatformBinding::install(lua_State* L)
{
    struct Entry { const char* name; lua_CFunction fn; };
    const Entry entries[] = {
        {"MoveAbsInt32", MoveAbsInt32Wrapper},
        {"MoveAbsFloat", MoveAbsFloatWrapper},
        {"MoveRelativeInt32", MoveRelativeInt32Wrapper},
        {"MoveRelativeFloat", MoveRelativeFloatWrapper},
        {"WriteCurrentPosInt32", WriteCurrentPosInt32Wrapper},
        {"WriteCurrentPosFloat", WriteCurrentPosFloatWrapper},
    };
    for (const Entry& e : entries) {
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, e.fn, 1);
        lua_setglobal(L, e.name);
    }
}

static int StubVoid(lua_State*) { return 0; }

void PlatformBinding::installStubs(lua_State* L)
{
    const char* fns[] = {"MoveAbsInt32","MoveAbsFloat","MoveRelativeInt32",
                         "MoveRelativeFloat","WriteCurrentPosInt32","WriteCurrentPosFloat"};
    for (const char* n : fns) { lua_pushcfunction(L, StubVoid); lua_setglobal(L, n); }
}

QList<LuaFunctionDoc> PlatformBinding::functions() const
{
    return {
        {"MoveAbsInt32",  "MoveAbsInt32(\"D100\", \"D102\", \"D104\") --根据指定寄存器绝对移动,双字"},
        {"MoveAbsFloat",  "MoveAbsFloat(\"D100\", \"D102\", \"D104\") --根据指定寄存器绝对移动,浮点数"},
        {"MoveRelativeInt32", "MoveRelativeInt32(\"D100\", \"D102\", \"D104\") --根据指定寄存器相对移动,双字"},
        {"MoveRelativeFloat", "MoveRelativeFloat(\"D100\", \"D102\", \"D104\") --根据指定寄存器相对移动,浮点数"},
        {"WriteCurrentPosInt32", "WriteCurrentPosInt32(\"D100\", \"D102\", \"D104\") --写入当前位置,双字"},
        {"WriteCurrentPosFloat", "WriteCurrentPosFloat(\"D100\", \"D102\", \"D104\") --写入当前位置,浮点数"},
    };
}

int PlatformBinding::MoveAbsInt32Wrapper(lua_State* L)
{
    PlatformBinding* b = self_from(L);
    int x, y, a;
    if (!parseThreeAddr(L, x, y, a)) return 0;
    if (!b->m_controller) return 0;
    b->m_controller->MovePlatformAbsInt32(b->m_access->GetInt32(x), b->m_access->GetInt32(y), b->m_access->GetInt32(a));
    return 0;
}

int PlatformBinding::MoveAbsFloatWrapper(lua_State* L)
{
    PlatformBinding* b = self_from(L);
    int x, y, a;
    if (!parseThreeAddr(L, x, y, a)) return 0;
    if (!b->m_controller) return 0;
    b->m_controller->MovePlatformAbsFloat(b->m_access->GetFloat(x), b->m_access->GetFloat(y), b->m_access->GetFloat(a));
    return 0;
}

int PlatformBinding::MoveRelativeInt32Wrapper(lua_State* L)
{
    PlatformBinding* b = self_from(L);
    int x, y, a;
    if (!parseThreeAddr(L, x, y, a)) return 0;
    if (!b->m_controller) return 0;
    b->m_controller->MovePlatformRelativeInt32(b->m_access->GetInt32(x), b->m_access->GetInt32(y), b->m_access->GetInt32(a));
    return 0;
}

int PlatformBinding::MoveRelativeFloatWrapper(lua_State* L)
{
    PlatformBinding* b = self_from(L);
    int x, y, a;
    if (!parseThreeAddr(L, x, y, a)) return 0;
    if (!b->m_controller) return 0;
    b->m_controller->MovePlatformRelativeFloat(b->m_access->GetFloat(x), b->m_access->GetFloat(y), b->m_access->GetFloat(a));
    return 0;
}

int PlatformBinding::WriteCurrentPosInt32Wrapper(lua_State* L)
{
    PlatformBinding* b = self_from(L);
    int x, y, a;
    if (!parseThreeAddr(L, x, y, a)) return 0;
    if (!b->m_controller) return 0;
    int32_t cx, cy, ca;
    b->m_controller->GetCurrentPosInt32(cx, cy, ca);
    b->m_access->SetInt32(x, cx);
    b->m_access->SetInt32(y, cy);
    b->m_access->SetInt32(a, ca);
    return 0;
}

int PlatformBinding::WriteCurrentPosFloatWrapper(lua_State* L)
{
    PlatformBinding* b = self_from(L);
    int x, y, a;
    if (!parseThreeAddr(L, x, y, a)) return 0;
    if (!b->m_controller) return 0;
    double cx, cy, ca;
    b->m_controller->GetCurrentPosFloat(cx, cy, ca);
    b->m_access->SetFloat(x, static_cast<float>(cx));
    b->m_access->SetFloat(y, static_cast<float>(cy));
    b->m_access->SetFloat(a, static_cast<float>(ca));
    return 0;
}
