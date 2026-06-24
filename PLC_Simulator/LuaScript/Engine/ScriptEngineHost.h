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

class RegisterStore;
class LuaEngine;
class QMutex;
class QThreadPool;

// Lua 子系统拥有者:引擎 + 业务绑定 + 线程池/锁 + 执行。
class ScriptEngineHost : public QObject
{
    Q_OBJECT
public:
    explicit ScriptEngineHost(RegisterStore* store, int engineCount = 6, QObject* parent = nullptr);
    ~ScriptEngineHost() override;

    // 外部构造的绑定注册进本宿主:install 进所有已建引擎 + 纳入文档清单。须在脚本运行前调用。
    void installModule(std::unique_ptr<ILuaBinding> module);

    // 文件路径异步执行(面板/QuickPanel);完成后 emit scriptFinished(失败再转 scriptLog)
    void runScript(int index, const QString& luaFile);

    // 同步语法检查(GUI 线程):仅编译+字节码扫描,检测语法错与未定义全局
    bool checkScript(const QString& script, QString& errorMsg) const;
    // 编辑器内容异步执行;完成后 emit scriptFinished(index, ok, err),调用方据此收尾。
    void runScriptAsync(int index, const QString& content);

    LuaEngine* engine(int index) const;
    QList<LuaFunctionDoc> functionDocs() const;   // 引擎内建 + 各模块
    void setLoopValid(int index, bool valid);

signals:
    void scriptLog(QString msg);
    // 任一池任务完成(成功/失败)后发出,index 为引擎索引;跨线程自动排队到接收者线程。
    void scriptFinished(int index, bool ok, QString err);

private:
    // 在线程池上执行 exec(工作线程);完成后统一 emit scriptFinished。
    void runOnPool(int index, std::function<bool(LuaEngine*, QString&)> exec);

    RegisterStore* m_store;
    int m_engineCount;
    std::vector<std::unique_ptr<LuaEngine>> m_engines;
    std::vector<std::unique_ptr<QMutex>> m_mutexes;
    QThreadPool* m_threadPool;
    std::vector<std::unique_ptr<ILuaBinding>> m_modules;  // register + platform
};

#endif // SCRIPTENGINEHOST_H
