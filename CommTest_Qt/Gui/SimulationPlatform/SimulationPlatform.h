/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef SIMULATIONPLATFORM_H
#define SIMULATIONPLATFORM_H

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QPointF>
#include <QPoint>
#include <QMap>
#include <QTimer>
#include <QPalette>
#include <QScreen>
#include <QFont>
#include <QMainWindow>
#include <QStackedWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QStatusBar>
#include <QDialog>
#include <QPushButton>
#include <QImage>
#include <QWheelEvent>
#include <QMouseEvent>
#include <cmath>

// 包含拆分出的控件头文件
#include "PlatformCanvas.h"
#include "ImagePage.h"
#include "CollapsibleGroupBox.h"
#include "PlatformScene.h"
#include "PlatformControlPanel.h"
#include "PlatformParamsDialog.h"

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

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // UI控件
    PlatformCanvas* canvas;

    // 右侧控制面板
    PlatformControlPanel* m_controlPanel = nullptr;

    // 页面/菜单/状态栏
    QStackedWidget* stack;
    QAction* actPageSim;
    QAction* actPagePic;
    QMenu* simMenu;
    QMenu* imageMenu;
    QSize m_pageSize[2];   // 各页窗口尺寸的会话内记忆(0=模拟页 1=图片页),不落盘
    void buildMenuBar();
    void showPage(int index);
    void moveToScreenCorner(int corner);
    void bindGroupToggle(CollapsibleGroupBox* g, const QString& title, bool visible = true);
    void openParamDialog();

    // 页面切换相关
    QWidget* simulationPage;      // 模拟平台页面
    ImagePage* m_imagePage = nullptr;  // 图片显示页面

    // 状态栏
    QLabel* statusLeft;
    QLabel* statusRight;
    void updateStatusBarForPage(int index);

    // 场景数据模型(单一数据源)
    PlatformScene* m_scene = nullptr;

    void setupUI();
};

#endif // SIMULATIONPLATFORM_H
