/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef LUASCRIPT_H
#define LUASCRIPT_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>
#include "Lua.hpp"

class ILuaBinding;  // 前向声明，避免 ILuaBinding.h 泄漏

class LuaScript :public QObject
{
	Q_OBJECT
public:
	//构造函数
    ~LuaScript() ;

	//禁用拷贝构造&赋值构造
    LuaScript(const LuaScript&) = delete;
    LuaScript& operator=(const LuaScript&) = delete;

    static LuaScript* InitialLuaScript(QObject* pParent = nullptr);

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
	LuaScript(QObject* parent = nullptr);

	lua_State* m_pLua;

	bool m_bLoopValid;	//循环是否有效

	bool RegisterLuaFunc();

private:
	//注册到Lua的循环状态判断函数
	static int IsLoopValidWrapper(lua_State* L);
	static int SleepWrapper(lua_State* L);

private:
	static lua_State* g_LuaCompileState;	//脚本编译检查状态机
	static void InitialCompileLuaState();	//初始化脚本编译检查状态机

public:
	static void ReleaseCompileLuaState();	//释放脚本编译检查状态机
	static bool CheckLuaScript(const QString& strLuaFile,QString& strErrorInfo);	//检查脚本是否正常
	static QStringList getRegisteredFunctions() {
        // 返回所有已注册到Lua状态机的函数列表
        return QStringList() << "SetInt16" << "SetInt32" << "SetFloat" << "SetDouble" << "SetString"
                            << "GetInt16" << "GetInt32" << "GetFloat" << "GetDouble" << "GetString"
                            << "IsLoopValid"<< "sleep"
                            << "MoveAbsInt32" << "MoveAbsFloat" << "MoveRelativeInt32" << "MoveRelativeFloat"
							<< "WriteCurrentPosInt32" << "WriteCurrentPosFloat";
    }
};

#endif	// LUASCRIPT_H


