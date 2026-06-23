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
	// 仅注册引擎内建函数：IsLoopValid 与 sleep
	lua_pushlightuserdata(m_pLua, this);
	lua_pushcclosure(m_pLua, IsLoopValidWrapper, 1);
	lua_setglobal(m_pLua, "IsLoopValid");

	lua_pushlightuserdata(m_pLua, this);
	lua_pushcclosure(m_pLua, SleepWrapper, 1);
	lua_setglobal(m_pLua, "sleep");

	return true;
}

void LuaEngine::install(ILuaBinding& binding)
{
	binding.install(m_pLua);
}

// ===== 循环状态函数实现 =====
int LuaEngine::IsLoopValidWrapper(lua_State* L)
{
	LuaEngine* pThis = static_cast<LuaEngine*>(lua_touserdata(L, lua_upvalueindex(1)));

	bool bValid = pThis->GetLoopValid();
	lua_pushboolean(L, bValid);
	return 1;
}

int LuaEngine::SleepWrapper(lua_State* L)
{
	LuaEngine* pThis = static_cast<LuaEngine*>(lua_touserdata(L, lua_upvalueindex(1)));

	// 获取参数
	if (!lua_isnumber(L, 1)) {
		return luaL_error(L, "Argument #1 (milliseconds) must be a number");
	}
	int milliseconds = lua_tointeger(L, 1);

	/*QThread::msleep(milliseconds);*/
	QEventLoop loop;
	QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
	loop.exec();
	return 0;
}
