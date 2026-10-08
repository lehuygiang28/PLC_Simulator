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

#include <atomic>

struct lua_State;
struct lua_Debug;

#include "ILuaBinding.h"  // ILuaBinding(install 形参) / LuaFunctionDoc(builtinFunctionDocs)

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

	void SetLoopValid(bool bValid) { m_bLoopValid = bValid; }
	bool GetLoopValid() const { return m_bLoopValid.load(); }

	void RequestStop() { m_stopRequested.store(true); }
	void ClearStopRequest() { m_stopRequested.store(false); }
	bool StopRequested() const { return m_stopRequested.load(); }

	void PrepareForRun(bool loopValid);

	bool RunLuaScript(const QString& strLuaFile,QString& errorMsg);
	bool RunLuaScriptWithEditor(const QString& strLuaContent,QString& errorMsg);

	// 安装一个绑定到本引擎 lua_State
	void install(ILuaBinding& binding);

private:
	//构造函数
	LuaEngine(QObject* parent = nullptr);

	lua_State* m_pLua;

	std::atomic<bool> m_bLoopValid;
	std::atomic<bool> m_stopRequested;

	static void abortHook(lua_State* L, lua_Debug* ar);
	void setAbortHookEnabled(bool enabled);

	bool RegisterLuaFunc();

	// 统一执行核:从缓冲加载(chunkName 决定错误信息里的来源名/行号) + pcall + 错误处理
	bool runChunk(const QByteArray& code, const QByteArray& chunkName, QString& errorMsg);

public:
	// 引擎内建函数(IsLoopValid/sleep)的文档——供 ScriptEngineHost 聚合
	static QList<LuaFunctionDoc> builtinFunctionDocs();

};

#endif	// LUAENGINE_H


