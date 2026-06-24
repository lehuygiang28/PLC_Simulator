/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "Lua.hpp"
#include "LuaEngine.h"
#include "ILuaBinding.h"

namespace {
// 内建真实实现(upvalue 为 LuaEngine*)
int IsLoopValidReal(lua_State* L)
{
	LuaEngine* pThis = static_cast<LuaEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
	lua_pushboolean(L, pThis ? pThis->GetLoopValid() : 0);
	return 1;
}
int SleepReal(lua_State* L)
{
	if (!lua_isnumber(L, 1)) {
		return luaL_error(L, "Argument #1 (milliseconds) must be a number");
	}
	int milliseconds = lua_tointeger(L, 1);
	QEventLoop loop;
	QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
	loop.exec();
	return 0;
}

// 内建单一来源:注册/文档均由此表派生
struct Builtin { const char* name; lua_CFunction real; const char* snippet; };
const Builtin kBuiltins[] = {
	{"IsLoopValid", IsLoopValidReal, "IsLoopValid() -- 获取循环是否有效"},
	{"sleep",       SleepReal,       "sleep(500) -- 睡眠500毫秒"},
};
} // namespace

static int nLuaEngineNum = 0;	// 引擎数量
LuaEngine* LuaEngine::InitialEngine(QObject* pParent /*= nullptr*/)
{
	nLuaEngineNum++;

	return new LuaEngine(pParent);
}

bool LuaEngine::RunLuaScript(const QString& strLuaFile,QString& errorMsg)
{
	QByteArray filePathData = strLuaFile.toLocal8Bit();
	if (luaL_dofile(m_pLua, filePathData.constData()) != LUA_OK)
	{
		const char* error_msg = lua_tostring(m_pLua, -1);
		lua_pop(m_pLua, 1); // 弹出错误信息
		errorMsg = QString::fromUtf8(error_msg);
		return false;
	}

	return true;
}

bool LuaEngine::RunLuaScriptWithEditor(const QString &strLuaContent,QString& errorMsg)
{
	QByteArray contentData = strLuaContent.toLocal8Bit();
	if (luaL_dostring(m_pLua, contentData.constData()) != LUA_OK)
	{
		const char* error_msg = lua_tostring(m_pLua, -1);
		lua_pop(m_pLua, 1); // 弹出错误信息

		errorMsg = QString::fromUtf8(error_msg);
		return false;
	}

    return true;
}

LuaEngine::~LuaEngine()
{
	if (m_pLua)
	{
        lua_close(m_pLua);
		m_pLua = nullptr;
		nLuaEngineNum--;
	}

}

LuaEngine::LuaEngine(QObject* parent /*= nullptr*/)
	:QObject(parent),
	m_pLua(luaL_newstate()),
	m_bLoopValid(false)
{
	luaL_openlibs(m_pLua);

	RegisterLuaFunc();
}

bool LuaEngine::RegisterLuaFunc()
{
	for (const Builtin& b : kBuiltins) {
		lua_pushlightuserdata(m_pLua, this);
		lua_pushcclosure(m_pLua, b.real, 1);
		lua_setglobal(m_pLua, b.name);
	}
	return true;
}

void LuaEngine::install(ILuaBinding& binding)
{
	binding.install(m_pLua);
}

// ===== 新静态接口 =====
QList<LuaFunctionDoc> LuaEngine::builtinFunctionDocs()
{
	QList<LuaFunctionDoc> docs;
	for (const Builtin& b : kBuiltins) {
		docs.append({QString::fromUtf8(b.name), QString::fromUtf8(b.snippet)});
	}
	return docs;
}

