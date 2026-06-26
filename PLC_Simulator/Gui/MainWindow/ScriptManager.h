/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include <QObject>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>

#include "ScriptEditor/ScriptEditor.h"

class ScriptEngineHost;

// 脚本管理器:管理 Lua 脚本的执行、编辑和循环控制。
// 20251225 wm 从 MainWindow.cpp 拆分,专注脚本相关逻辑。
class ScriptManager : public QObject
{
    Q_OBJECT

signals:
    // 脚本相关消息(如脚本不存在),转发到主界面日志
    void logMessage(const QString& message);

public:
    // host: 脚本引擎宿主指针; parent: 父窗口
    explicit ScriptManager(ScriptEngineHost* host, QWidget* parent = nullptr);

    ~ScriptManager() override;

    // 装配一行脚本控件(index 0-based;任一控件可为 null,各自跳过)
    // 执行按钮→runScript;编辑按钮→openScriptEditor;循环复选框→setLoopValid
    void bindScriptRow(int index, QPushButton* execBtn, QPushButton* editBtn, QCheckBox* loopChk);

    // 打开脚本编辑器(index 0-based)
    void openScriptEditor(int index);

    // 执行指定脚本(统一执行入口:主面板按钮与子窗口共用)
    // index 0-based;文件不存在则弹窗并发 logMessage
    void runScript(int index);

private:
    // 脚本索引(0-based)→ Lua 文件绝对路径(唯一拼路径处:LuaFile{index+1}.lua)
    QString luaPath(int index) const;

    // 处理已有编辑器:同脚本→置前;不同→提示保存/丢弃/取消并销毁旧的。
    // 返回 true=应继续打开新编辑器;false=已处理(置前或取消),调用方直接返回。
    bool prepareForNewEditor(int index);

    // 文本文件读写(统一 UTF-8);readTextFile 打不开返回 ""
    QString readTextFile(const QString& path) const;
    void writeTextFile(const QString& path, const QString& text) const;

private:
    ScriptEngineHost* m_host;              // 脚本引擎宿主指针
    QWidget* m_parentWidget;               // 父窗口
    ScriptEditor* m_pCurrentScriptEditor;  // 当前脚本编辑器
    int m_nCurrentScriptIndex;             // 当前编辑的脚本索引
};

#endif // SCRIPTMANAGER_H
