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
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>
#include <QVector>

#include "ScriptEditor/ScriptEditor.h"
#include "LuaScript/Engine/ScriptLanguage.h"

class ScriptEngineHost;
class ConfigStore;

// 脚本管理器:管理 Lua 脚本的执行、编辑和循环控制。
// 20251225 wm 从 MainWindow.cpp 拆分,专注脚本相关逻辑。
// TypeScript scripts are transpiled to Lua before execution.
class ScriptManager : public QObject
{
    Q_OBJECT

signals:
    void logMessage(const QString& message);
    void scriptPhaseChanged(int index, ScriptRunPhase phase);
    void scriptCompileStarted(int index);
    void scriptCompileFinished(int index);

public:
    explicit ScriptManager(ScriptEngineHost* host, ConfigStore* config, QWidget* parent = nullptr);
    ~ScriptManager() override;

    void setScriptCount(int count);

    void bindScriptRow(int index, QPushButton* execBtn, QPushButton* editBtn,
                       QCheckBox* loopChk, QComboBox* langCombo);

    void openScriptEditor(int index);
    void runScript(int index);

    ScriptLanguage language(int index) const;
    void setLanguage(int index, ScriptLanguage lang);

    void loadLanguagePrefs();
    void saveLanguagePrefs();
    void retranslateScriptRows();

    QString scriptPath(int index) const;
    ScriptRunPhase rowPhase(int index) const;

private:
    QString scriptDir() const;
    void runScriptWithContent(int index, const QString& editorContent);
    void dispatchLuaRun(int index, const QString& luaOut);
    void failRun(int index, const QString& title, const QString& errorMsg);
    void setRowPhase(int index, ScriptRunPhase phase);
    void updateExecuteButton(int index);

    bool resolveLuaSourceSync(int index, const QString& editorContent, QString& luaOut,
                              QString& errorMsg) const;
    bool checkLuaSource(ScriptLanguage lang, const QString& source, QString& errorMsg) const;

    bool prepareForNewEditor(int index);
    QString readTextFile(const QString& path) const;
    void writeTextFile(const QString& path, const QString& text) const;

private:
    ScriptEngineHost* m_host;
    ConfigStore* m_config;
    QWidget* m_parentWidget;
    ScriptEditor* m_pCurrentScriptEditor;
    int m_nCurrentScriptIndex;
    int m_scriptCount = 0;
    QVector<ScriptLanguage> m_languages;
    QVector<QComboBox*> m_langCombos;
    QVector<QPushButton*> m_execBtns;
    QVector<ScriptRunPhase> m_rowPhase;
};

#endif // SCRIPTMANAGER_H
