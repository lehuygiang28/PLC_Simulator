/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef LUAENGINE_H
#define LUAENGINE_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>

struct lua_State;  // 前向声明，避免 Lua.hpp 泄漏到包含方

class ILuaBinding;  // 前向声明，避免 ILuaBinding.h 泄漏

class LuaEngine :public QObject
{
	Q_OBJECT
public:
	//构造函数
    ~LuaEngine() ;

	//禁用拷贝构造&赋值构造
    LuaEngine(const LuaEngine&) = delete;
    LuaEngine& operator=(const LuaEngine&) = delete;

    static LuaEngine* InitialEngine(QObject* pParent = nullptr);

	//设置循环是否有效
	void SetLoopValid(bool bValid) {
		m_bLoopValid = bValid;
	}

	// 获取循环是否有效
	bool GetLoopValid() const {
		return m_bLoopValid;
	}

	bool RunLuaScript(const QString& strLuaFile,QString& errorMsg);
	bool RunLuaScriptWithEditor(const QString& strLuaContent,QString& errorMsg);

	// 安装一个绑定到本引擎 lua_State
	void install(ILuaBinding& binding);

private:
	//构造函数
	LuaEngine(QObject* parent = nullptr);

	lua_State* m_pLua;

	bool m_bLoopValid;	//循环是否有效

	bool RegisterLuaFunc();

private:
	//注册到Lua的循环状态判断函数
	static int IsLoopValidWrapper(lua_State* L);
	static int SleepWrapper(lua_State* L);

};

#endif	// LUAENGINE_H


