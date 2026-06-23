/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "LuaScript.h"
#include "ILuaBinding.h"


lua_State* LuaScript::g_LuaCompileState = nullptr;
static int nLuaScriptNum = 0;	// 脚本数量
LuaScript* LuaScript::InitialLuaScript(QObject* pParent /*= nullptr*/)
{
	InitialCompileLuaState();
	nLuaScriptNum++;

	return new LuaScript(pParent);
}

bool LuaScript::RunLuaScript(const QString& strLuaFile,QString& errorMsg)
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

bool LuaScript::RunLuaScriptWithEditor(const QString &strLuaContent,QString& errorMsg)
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

LuaScript::~LuaScript()
{
	if (m_pLua)
	{
        lua_close(m_pLua);
		m_pLua = nullptr;
		nLuaScriptNum--;
	}

	// g_LuaCompileState 由 ReleaseCompileLuaState() 统一释放
	// 不在析构函数中自动释放,避免多线程/析构顺序问题
}

LuaScript::LuaScript(QObject* parent /*= nullptr*/)
	:QObject(parent),
	m_pLua(luaL_newstate()),
	m_bLoopValid(false)
{
	luaL_openlibs(m_pLua);

	RegisterLuaFunc();
}

bool LuaScript::RegisterLuaFunc()
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

void LuaScript::install(ILuaBinding& binding)
{
	binding.install(m_pLua);
}

// ===== 循环状态函数实现 =====
int LuaScript::IsLoopValidWrapper(lua_State* L)
{
	LuaScript* pThis = static_cast<LuaScript*>(lua_touserdata(L, lua_upvalueindex(1)));

	bool bValid = pThis->GetLoopValid();
	lua_pushboolean(L, bValid);
	return 1;
}

int LuaScript::SleepWrapper(lua_State* L)
{
	LuaScript* pThis = static_cast<LuaScript*>(lua_touserdata(L, lua_upvalueindex(1)));

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

// 静态C函数用于编译检查
static int DummyLuaFunc(lua_State* L) {
	return 0;
}

static int DummyLuaFuncReturn1(lua_State* L) {
	lua_pushinteger(L, 0);
	return 1;
}

void LuaScript::InitialCompileLuaState()
{
	static std::once_flag initFlag;

	auto onceFunc = [](){
	//检查全局静态状态机是否为空
	if (g_LuaCompileState == nullptr)
	{
		g_LuaCompileState = luaL_newstate();
		if (g_LuaCompileState == nullptr)
		{
			return;
		}
		luaL_openlibs(g_LuaCompileState);

		//注册Set系列函数
		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "SetInt16");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "SetInt32");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "SetFloat");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "SetDouble");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "SetString");

		//注册Get系列函数
		lua_pushcfunction(g_LuaCompileState, DummyLuaFuncReturn1);
		lua_setglobal(g_LuaCompileState, "GetInt16");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFuncReturn1);
		lua_setglobal(g_LuaCompileState, "GetInt32");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFuncReturn1);
		lua_setglobal(g_LuaCompileState, "GetFloat");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFuncReturn1);
		lua_setglobal(g_LuaCompileState, "GetDouble");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFuncReturn1);
		lua_setglobal(g_LuaCompileState, "GetString");

		//注册循环状态函数
		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "IsLoopValid");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "sleep");
		//注册平台控制函数
		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "MoveAbsInt32");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "MoveAbsFloat");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "MoveRelativeInt32");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "MoveRelativeFloat");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "WriteCurrentPosInt32");

		lua_pushcfunction(g_LuaCompileState, DummyLuaFunc);
		lua_setglobal(g_LuaCompileState, "WriteCurrentPosFloat");
		 //qDebug() << "Compilation Lua state initialized";
	}};

	std::call_once(initFlag, onceFunc);
}

void LuaScript::ReleaseCompileLuaState()
{
	if (g_LuaCompileState != nullptr)
	{
		lua_close(g_LuaCompileState);
		g_LuaCompileState = nullptr;
	}
}

bool LuaScript::CheckLuaScript(const QString &strLuaFile,QString& strErrorInfo)
{
    if (g_LuaCompileState == nullptr)	return false;

	//if (strLuaFile.isEmpty()) return false;

	QByteArray scriptData = strLuaFile.toLatin1();
	if(luaL_dostring(g_LuaCompileState, scriptData.data()) != LUA_OK)
	{
		const char* error_msg = lua_tostring(g_LuaCompileState, -1);
		lua_pop(g_LuaCompileState, 1);

		strErrorInfo = QString::fromUtf8(error_msg);

		return false;
	}

	return true;

}
