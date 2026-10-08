/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "Lua.hpp"
#include "LuaEngine.h"
#include <QCoreApplication>
#include "ILuaBinding.h"

#include <QFile>
#include <QByteArray>
#include <QtGlobal>

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
	LuaEngine* pThis = static_cast<LuaEngine*>(lua_touserdata(L, lua_upvalueindex(1)));
	int milliseconds = lua_tointeger(L, 1);
	const int stepMs = 50;
	while (milliseconds > 0) {
		if (pThis && pThis->StopRequested())
			return luaL_error(L, "script stopped by user");
		const int slice = qMin(milliseconds, stepMs);
		QEventLoop loop;
		QTimer::singleShot(slice, &loop, &QEventLoop::quit);
		loop.exec();
		milliseconds -= slice;
	}
	return 0;
}

// 内建单一来源:注册/文档均由此表派生
struct Builtin { const char* name; lua_CFunction real; const char* snippet; const char* description; };
const Builtin kBuiltins[] = {
	{"IsLoopValid", IsLoopValidReal, "IsLoopValid()", "获取循环是否有效"},
	{"sleep",       SleepReal,       "sleep(500)",    "睡眠,毫秒"},
};
} // namespace

static int nLuaEngineNum = 0;	// 引擎数量
LuaEngine* LuaEngine::InitialEngine(QObject* pParent /*= nullptr*/)
{
	nLuaEngineNum++;

	return new LuaEngine(pParent);
}

namespace {
constexpr const char kEngineRegistryKey[] = "PLCSimulator.LuaEngine";
constexpr int kAbortHookInstructionCount = 1000;
} // namespace

void LuaEngine::abortHook(lua_State* L, lua_Debug* ar)
{
	if (!ar || ar->event != LUA_HOOKCOUNT)
		return;
	lua_getfield(L, LUA_REGISTRYINDEX, kEngineRegistryKey);
	LuaEngine* eng = static_cast<LuaEngine*>(lua_touserdata(L, -1));
	lua_pop(L, 1);
	if (eng && eng->StopRequested())
		luaL_error(L, "script stopped by user");
}

void LuaEngine::setAbortHookEnabled(bool enabled)
{
	if (!m_pLua)
		return;
	if (enabled) {
		lua_pushlightuserdata(m_pLua, this);
		lua_setfield(m_pLua, LUA_REGISTRYINDEX, kEngineRegistryKey);
		lua_sethook(m_pLua, &LuaEngine::abortHook, LUA_MASKCOUNT, kAbortHookInstructionCount);
	} else {
		lua_sethook(m_pLua, nullptr, 0, 0);
	}
}

void LuaEngine::PrepareForRun(bool loopValid)
{
	ClearStopRequest();
	SetLoopValid(loopValid);
}

bool LuaEngine::runChunk(const QByteArray& code, const QByteArray& chunkName, QString& errorMsg)
{
	setAbortHookEnabled(true);
	int status = luaL_loadbuffer(m_pLua, code.constData(), static_cast<size_t>(code.size()),
	                             chunkName.constData());
	if (status == LUA_OK)
		status = lua_pcall(m_pLua, 0, LUA_MULTRET, 0);
	setAbortHookEnabled(false);
	if (status != LUA_OK)
	{
		errorMsg = QString::fromUtf8(lua_tostring(m_pLua, -1));
		lua_pop(m_pLua, 1);
		if (errorMsg.contains(QStringLiteral("script stopped by user"))) {
			errorMsg.clear();
			return true;
		}
		return false;
	}
	return true;
}

bool LuaEngine::RunLuaScript(const QString& strLuaFile,QString& errorMsg)
{
	QFile file(strLuaFile);  // QFile 原生支持中文路径,避开 fopen/ANSI
	if (!file.open(QIODevice::ReadOnly))
	{
		errorMsg = QCoreApplication::translate("LuaEngine", "无法打开脚本文件: %1").arg(strLuaFile);
		return false;
	}
	QByteArray code = file.readAll();  // 磁盘按 UTF-8 保存(ScriptManager 约定)
	if (code.startsWith("\xEF\xBB\xBF")) code.remove(0, 3);  // 剥离可能的 UTF-8 BOM
	// chunk 名以 "@" 前缀标记为文件名 → 错误信息显示 "路径:行号"
	return runChunk(code, "@" + strLuaFile.toUtf8(), errorMsg);
}

bool LuaEngine::RunLuaScriptWithEditor(const QString &strLuaContent,QString& errorMsg)
{
	// 编辑器内容统一按 UTF-8(与文件保存/校验器/RegisterBinding::fromUtf8 一致)
	return runChunk(strLuaContent.toUtf8(), QByteArrayLiteral("@[editor]"), errorMsg);
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
	m_bLoopValid(false),
	m_stopRequested(false)
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
		docs.append({QString::fromUtf8(b.name), QString::fromUtf8(b.snippet),
		             QCoreApplication::translate("LuaEngine", b.description)});
	}
	return docs;
}

