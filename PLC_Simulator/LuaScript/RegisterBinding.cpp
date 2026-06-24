/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "RegisterBinding.h"
#include "IRegisterAccess.h"
#include "LuaBindingUtil.h"
#include "Lua.hpp"

#include <QString>
#include <cfloat>
#include <cstdint>
#include <cstdio>

RegisterBinding::RegisterBinding(IRegisterAccess* access)
    : m_access(access)
{
}

static RegisterBinding* self_from(lua_State* L)
{
    return static_cast<RegisterBinding*>(lua_touserdata(L, lua_upvalueindex(1)));
}

// 各 wrapper 共用前导:取 binding、校验 arg1 为地址字符串、解析出 nAddr。
// 出错时 luaL_error 直接 longjmp,不返回。
static RegisterBinding* prologue(lua_State* L, int& nAddr)
{
    RegisterBinding* b = self_from(L);
    if (!lua_isstring(L, 1)) luaL_error(L, "Argument #1 must be a string (register address)");
    const char* addr = lua_tostring(L, 1);
    if (!LuaBindingUtil::parseRegisterAddr(addr, nAddr)) luaL_error(L, "Register address Invalid: %s", addr);
    return b;
}

void RegisterBinding::install(lua_State* L)
{
    struct Entry { const char* name; lua_CFunction fn; };
    const Entry entries[] = {
        {"SetInt16", SetInt16Wrapper}, {"SetInt32", SetInt32Wrapper},
        {"SetFloat", SetFloatWrapper}, {"SetDouble", SetDoubleWrapper},
        {"SetString", SetStringWrapper},
        {"GetInt16", GetInt16Wrapper}, {"GetInt32", GetInt32Wrapper},
        {"GetFloat", GetFloatWrapper}, {"GetDouble", GetDoubleWrapper},
        {"GetString", GetStringWrapper},
    };
    for (const Entry& e : entries) {
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, e.fn, 1);
        lua_setglobal(L, e.name);
    }
}

QList<LuaFunctionDoc> RegisterBinding::functions() const
{
    return {
        {"SetInt16",  "SetInt16(\"D100\", 123) -- 设置D100为123,单字"},
        {"SetInt32",  "SetInt32(\"D100\", 123)    -- 设置D100为123,双字"},
        {"SetFloat",  "SetFloat(\"D100\", 123.45) -- 设置D100为123.45,浮点数"},
        {"SetDouble", "SetDouble(\"D100\", 123.45) -- 设置D100为123.45,双精度浮点数"},
        {"SetString", "SetString(\"D100\", \"AB\") -- 设置D100为字符串AB"},
        {"GetInt16",  "GetInt16(\"D100\") -- 获取D100的值,单字"},
        {"GetInt32",  "GetInt32(\"D100\")   -- 获取D100的值,双字"},
        {"GetFloat",  "GetFloat(\"D100\") -- 获取D100的值,浮点数"},
        {"GetDouble", "GetDouble(\"D100\") -- 获取D100的值,双精度浮点数"},
        {"GetString", "GetString(\"D100\") -- 获取D100的值,字符串"},
    };
}

int RegisterBinding::SetInt16Wrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    lua_Integer value = luaL_checkinteger(L, 2);
    if (value < INT16_MIN || value > INT16_MAX) {
        char msg[128];
        std::snprintf(msg, sizeof(msg), "Value %lld out of int16 range [%d, %d]",
                      static_cast<long long>(value), INT16_MIN, INT16_MAX);
        return luaL_error(L, "%s", msg);
    }
    b->m_access->SetInt16(nAddr, static_cast<int16_t>(value));
    return 0;
}

int RegisterBinding::SetInt32Wrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    lua_Integer value = luaL_checkinteger(L, 2);
    if (value < INT32_MIN || value > INT32_MAX) {
        char msg[128];
        std::snprintf(msg, sizeof(msg), "Value %lld out of int32 range [%d, %d]",
                      static_cast<long long>(value), INT32_MIN, INT32_MAX);
        return luaL_error(L, "%s", msg);
    }
    b->m_access->SetInt32(nAddr, static_cast<int32_t>(value));
    return 0;
}

int RegisterBinding::SetFloatWrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    float fValue = static_cast<float>(luaL_checknumber(L, 2));
    if (fValue < -FLT_MAX || fValue > FLT_MAX) return luaL_error(L, "Value %f out of float range [-%f, %f]", fValue, -FLT_MAX, FLT_MAX);
    b->m_access->SetFloat(nAddr, fValue);
    return 0;
}

int RegisterBinding::SetDoubleWrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    double dValue = static_cast<double>(luaL_checknumber(L, 2));
    if (dValue < -DBL_MAX || dValue > DBL_MAX) return luaL_error(L, "Value %f out of double range [-%f, %f]", dValue, -DBL_MAX, DBL_MAX);
    b->m_access->SetDouble(nAddr, dValue);
    return 0;
}

int RegisterBinding::SetStringWrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    if (!lua_isstring(L, 2)) return luaL_error(L, "Argument #2 must be a string (register value)");
    const char* strValue = lua_tostring(L, 2);
    QString strVal = QString::fromUtf8(strValue);
    if (strVal.length() > 2) strVal = strVal.left(2);
    b->m_access->SetString(nAddr, strVal);
    return 0;
}

int RegisterBinding::GetInt16Wrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    lua_pushinteger(L, b->m_access->GetInt16(nAddr));
    return 1;
}

int RegisterBinding::GetInt32Wrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    lua_pushinteger(L, b->m_access->GetInt32(nAddr));
    return 1;
}

int RegisterBinding::GetFloatWrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    lua_pushnumber(L, b->m_access->GetFloat(nAddr));
    return 1;
}

int RegisterBinding::GetDoubleWrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    lua_pushnumber(L, b->m_access->GetDouble(nAddr));
    return 1;
}

int RegisterBinding::GetStringWrapper(lua_State* L)
{
    int nAddr = 0;
    RegisterBinding* b = prologue(L, nAddr);
    QString value = b->m_access->GetString(nAddr);
    QByteArray utf8 = value.toUtf8();
    lua_pushstring(L, utf8.constData());
    return 1;
}
