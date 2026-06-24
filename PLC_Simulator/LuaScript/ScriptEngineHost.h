/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef SCRIPTENGINEHOST_H
#define SCRIPTENGINEHOST_H

#include <QObject>
#include <QList>
#include <QString>
#include <vector>
#include <memory>
#include <functional>

#include "ILuaBinding.h"                     // LuaFunctionDoc / ILuaBinding
#include "Gui/ScriptEditor/ScriptEditor.h"   // IScriptRunner

class IRegisterAccess;
class IPlatformController;
class LuaEngine;
class LuaSyntaxChecker;
class PlatformBinding;
class QMutex;
class QThreadPool;

// Lua 子系统拥有者:6 引擎 + 业务绑定 + 校验器 + runner + 线程池/锁 + 执行。
// 对外仅注入 IRegisterAccess* 与 IPlatformController*。
class ScriptEngineHost : public QObject
{
    Q_OBJECT
public:
    explicit ScriptEngineHost(IRegisterAccess* registerAccess, int engineCount = 6, QObject* parent = nullptr);
    ~ScriptEngineHost() override;

    void setPlatformController(IPlatformController* controller);

    // 文件路径异步执行(面板/QuickPanel);成功后 notifyChanged,失败 emit scriptLog
    bool runScript(int index, const QString& luaFile);
    // 编辑器内容异步执行(供 ScriptRunnerImpl 转发);结果走回调
    void runEditorScriptAsync(int index, const QString& content,
                              std::function<void(bool, const QString&)> onFinished);

    // 同步语法检查(GUI 线程):仅编译+字节码扫描,检测语法错与未定义全局
    bool checkScript(const QString& script, QString& errorMsg) const;
    // 编辑器内容异步执行(带完成回调)
    void runScriptAsync(int index, const QString& content,
                        std::function<void(bool, const QString&)> onFinished);

    LuaEngine* engine(int index) const;
    IScriptRunner* scriptRunner(int index) const;
    LuaSyntaxChecker* syntaxChecker() const;
    QList<LuaFunctionDoc> functionDocs() const;   // 引擎内建 + 各模块
    void setLoopValid(int index, bool valid);

signals:
    void scriptLog(QString msg);

private:
    void runOnPool(int index,
                   std::function<bool(LuaEngine*, QString&)> exec,
                   std::function<void(bool, const QString&)> onFinished);

    IRegisterAccess* m_registerAccess;
    int m_engineCount;
    std::vector<std::unique_ptr<LuaEngine>> m_engines;
    std::vector<std::unique_ptr<QMutex>> m_mutexes;
    QThreadPool* m_threadPool;
    std::vector<std::unique_ptr<IScriptRunner>> m_runners;
    std::vector<std::unique_ptr<ILuaBinding>> m_modules;  // register + platform
    std::unique_ptr<LuaSyntaxChecker> m_checker;
    PlatformBinding* m_platformBinding = nullptr;         // 指向 m_modules 中的平台绑定,供 setPlatformController
};

#endif // SCRIPTENGINEHOST_H
