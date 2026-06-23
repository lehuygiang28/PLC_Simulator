/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef SIMULATIONPLATFORM_H
#define SIMULATIONPLATFORM_H

#include <QMainWindow>
#include <QSize>
#include <QString>
#include "PlatformTypes.h"   // 信号参数 Pose / Platform

// 成员均为指针,前向声明即可;完整定义在 .cpp 中包含
class QStackedWidget;
class QMenu;
class QAction;
class QLabel;
class PlatformCanvas;
class PlatformControlPanel;
class PlatformScene;
class ImagePage;
class CollapsibleGroupBox;

class SimulationPlatform : public QMainWindow
{
    Q_OBJECT
signals:
    void sceneParamsChanged(double markCenterDistance, double screenRatio);
    void poseChanged(Platform which, const Pose& pose);

public:
    explicit SimulationPlatform(QWidget *parent = nullptr);

    // 平台控制公共接口
    void moveAbsolute(const Pose& target, Platform which = Platform::Live);
    void moveRelative(const Pose& delta, Platform which = Platform::Live);

    // 获取平台位姿（Live 或 Base）
    Pose pose(Platform which = Platform::Live) const;

    void setSceneParams(double markCenterDistance, double screenRatio);

private:
    // UI控件
    PlatformCanvas* m_canvas;

    // 右侧控制面板
    PlatformControlPanel* m_controlPanel = nullptr;

    // 页面/菜单/状态栏
    QStackedWidget* m_stack;
    QAction* m_actPageSim;
    QAction* m_actPagePic;
    QMenu* m_simMenu;
    QMenu* m_imageMenu;
    QSize m_pageSize[2];   // 各页窗口尺寸的会话内记忆(0=模拟页 1=图片页),不落盘
    void buildMenuBar();
    void showPage(int index);
    void moveToScreenCorner(int corner);
    void bindGroupToggle(CollapsibleGroupBox* g, const QString& title, bool visible = true);
    void openParamDialog();

    // 页面切换相关
    QWidget* m_simulationPage;      // 模拟平台页面
    ImagePage* m_imagePage = nullptr;  // 图片显示页面

    // 状态栏
    QLabel* m_statusLeft;
    QLabel* m_statusRight;
    void updateStatusBarForPage(int index);

    // 场景数据模型(单一数据源)
    PlatformScene* m_scene = nullptr;

    void setupUI();
};

#endif // SIMULATIONPLATFORM_H
