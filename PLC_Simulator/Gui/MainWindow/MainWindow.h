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
#include "SimulationPlatform/PlatformTypes.h"
#include "RegisterTableManager.h"
#include "ScriptManager.h"
#include "PlatformController.h"
#include "AuxDialogs.h"
#include "Config/ConfigStore.h"
#include "MainWorkFlow.h"
#include "Theme/ThemeManager.h"

#include <QtWidgets/QMainWindow>
#include <QMap>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; };
class QAction;
class QTimer;
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

class SimulationPlatform;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // 初始化阶段(构造函数按此顺序调用)
    void createMembers();          // ① 创建所有成员对象
    void setupUiContent();         // ② 填充静态 UI(下拉框 / 只读 / 状态栏)
    void loadConfigs();            // ③ 读取持久化配置
    void setupInputValidators();   // ④ 输入校验(须晚于 loadConfigs)
    void applyThemePref();         // ⑤ 应用持久化主题(须先于建菜单)
    void connectSignals();         // ⑥ 连接所有信号槽
    void initialRefresh();         // ⑦ 首屏刷新表格

    // 信号连接分组(由 connectSignals 调用)
    void buildMenus();
    void connectWindowSignals();
    void connectRegisterTable();
    void connectComm();
    void connectLog();
    void connectScript();

    // 协议相关
    void CreateCurrentProtocol();

    // 平台控制相关
    void OnWriteAxisDoubleWord();
    void OnWriteAxisFloat();

    // 轴写入辅助
    bool axisStartAddrValid(int addr) const;
    void writeAxisPos(int startAddr, PlatformController::NumFormat fmt, Platform which);
    void writeAxisManual(PlatformController::NumFormat fmt);

    // 自动写入相关
    void OnPlatformPoseChanged(Platform which, const Pose& pose);

    // 平台参数相关
    void refreshAxisAddrStatus();

    // 菜单栏相关
    void OnThemeSelected(Theme theme);

    // 日志显示
    void UpdateLogDisplay(QString strNewLog);

    // 寄存器表相关
    void silentRefreshTable(int addr);

    // 脚本名称编辑框辅助
    QVector<QLineEdit*> scriptNameEdits() const;

private:
    Ui::MainWindow* ui;

    // 平台控制:参数数据成员
    AuxDialogs::PlatformParams m_platformParams{3, 3, 114, 120};
    QLabel* m_statusAddrLabel = nullptr;

    // 平台菜单 actions(跨槽访问的才做成员;仅菜单构建期用的为局部变量)
    QAction* m_actShowPlatform = nullptr;
    QAction* m_actAutoWrite = nullptr;
    QAction* m_actFmtFloat = nullptr;
    QAction* m_actFmtInt32 = nullptr;

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
