/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_MainWindow.h"
#include "QuickPanel.h"
#include "SimulationPlatform/SimulationPlatform.h"
#include "RegisterTableManager.h"
#include "ScriptManager.h"
#include "PlatformController.h"
#include "Config/ConfigStore.h"
#include "MainWorkFlow.h"
#include "Theme/ThemeManager.h"

#include <QtWidgets/QMainWindow>
#include <QColor>
#include <QTimer>
#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>
#include <QButtonGroup>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QDialog>
#include <QFrame>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; };
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // 初始化方法
    void InitializeMember();
    void InitialSignalConnect();
    void InitialLineEditValidator();
    void InitialAllConfigs();

    // 协议相关
    void CreateCurrentProtocol();

    // 平台控制相关
    void OnWriteAxisDoubleWord();
    void OnWriteAxisFloat();

    // 自动写入相关
    void OnWritePosAutoEnableChanged(int state);
    void OnPlatformPoseChanged(Platform which, const Pose& pose);

    // 菜单栏相关
    void OnShowAboutDialog();
    void OnShowChangeLog();
    void OnThemeSelected(Theme theme);

    // 日志显示
    void UpdateLogDisplay(QString strNewLog);

private:
    Ui::MainWindow* ui;
    QAction* m_actLightTheme = nullptr;
    QAction* m_actDarkTheme = nullptr;

    // 子窗口
    std::unique_ptr<QuickPanel> m_subWindow;
    SimulationPlatform* m_simulationPlatform;

    // 工作流
    MainWorkFlow* m_pWorkFlow;
    std::unique_ptr<PlatformController> m_platformController;

    // 配置管理
    ConfigStore* m_configStore;

    // 寄存器表格管理器
    std::unique_ptr<RegisterTableManager> m_registerTableManager;

    // 脚本管理器
    std::unique_ptr<ScriptManager> m_scriptManager;

    // 表格闪烁相关
    QMap<QTableWidgetItem*, QTimer*> m_animationTimers;
    QMap<QTableWidgetItem*, QString> m_lastTextValues;

    // 日志显示状态
    int m_nLogStat;

    // 通信日志重复帧过滤状态(与上一帧方向+端点+数据相同则不刷屏;收/发各自独立)
    QString    m_lastRecEndpoint;
    QByteArray m_lastRecData;
    QString    m_lastSendEndpoint;
    QByteArray m_lastSendData;
};

#endif // MAINWINDOW_H
