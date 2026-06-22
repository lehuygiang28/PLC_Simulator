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
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QButtonGroup>
#include <QGroupBox>
#include <QDoubleValidator>
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
#include "CanvasWidget.h"
#include "ImageViewerWidget.h"
#include "CollapsibleGroupBox.h"

class SimulationPlatform : public QMainWindow
{
    Q_OBJECT
signals:
    void parametersChanged(double markCenterDistance, double screenRatio);

public:
    explicit SimulationPlatform(QWidget *parent = nullptr);

    // 平台控制公共接口
    void SetRealTimePlatformAbs(double x, double y, double angle);  // 绝对位置移动
    void SetRealTimePlatformRelative(double x, double y, double angle);  // 相对位置移动

    //void SetSimulationPlatformParams(double distance,double ratio);
    
    // 获取实时平台数据
    void GetRealTimePlatformData(double& x, double& y, double& angle) const;

    // 获取基准平台数据
    void GetBasePlatformData(double& x, double& y, double& angle) const;

    void SetSimulationPlatformParams(double distance,double ratio);

    /**
     * @brief 供 CanvasWidget 调用的画布绘制方法
     * @param painter 绘图对象
     */
    void drawCanvas(QPainter& painter);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateBasePlatform();
    void updateRealTimePlatform();
    void updateMark1();
    void updateMark2();
    void updateVirtualMark();

private:
    // UI控件
    CanvasWidget* canvas;

    QLineEdit *basePlatformXEdit;
    QLineEdit *basePlatformYEdit;
    QLineEdit *basePlatformAngleEdit;
    QCheckBox* showBasePlatformCheckBox;
    
    QLineEdit *realTimePlatformXEdit;
    QLineEdit *realTimePlatformYEdit;
    QLineEdit *realTimePlatformAngleEdit;
    QCheckBox* showRealTimePlatformCheckBox;
    
    QLineEdit *mark1XEdit;
    QLineEdit *mark1YEdit;
    QLineEdit *mark1AngleEdit;
    QCheckBox *mark1FollowBaseCheckBox;
    QCheckBox *ShowMark1CheckBox;
    
    QLineEdit *mark2XEdit;
    QLineEdit *mark2YEdit;
    QLineEdit *mark2AngleEdit;
    QCheckBox *mark2FollowRealTimeCheckBox;
    QCheckBox *ShowMark2CheckBox;

    QLineEdit *virtualMarkXEdit;
    QLineEdit *virtualMarkYEdit;
    QCheckBox *showVirtualMarkCheckBox;
    
    // 5 个控制组(供菜单显隐/关闭引用)
    CollapsibleGroupBox* grpBase;
    CollapsibleGroupBox* grpRealTime;
    CollapsibleGroupBox* grpMark1;
    CollapsibleGroupBox* grpMark2;
    CollapsibleGroupBox* grpVirtual;
    void openParamDialog();

    // 页面/菜单/状态栏
    QStackedWidget* stack;
    QAction* actPageSim;
    QAction* actPagePic;
    QMenu* simMenu;
    QMenu* imageMenu;
    void buildMenuBar();
    void showPage(int index);
    void moveToScreenCorner(int corner);
    void bindGroupToggle(CollapsibleGroupBox* g, const QString& title);

    // 页面切换相关
    QWidget* simulationPage;      // 模拟平台页面
    QWidget* pictureShowPage;     // 图片显示页面 (PictureShow)

    // 图片显示页面控件
    ImageViewerWidget* imageViewer;

    void setupPictureShowPage();
    void loadDefaultImage();
    void onSetImageClicked();

    // 状态栏
    QLabel* statusLeft;
    QLabel* statusRight;
    QString m_imagePath;
    void updateStatusBarForPage(int index);

    // 数据
    struct Platform {
        double x;
        double y;
        double angle; // 角度（度）
    };
    
    struct Mark {
        double x;
        double y;
        double angle; // 角度（度）
        bool followPlatform;
    };
    
    Platform basePlatform;
    Platform realTimePlatform;
    Mark mark1;
    Mark mark2;
    Mark virtualMark;
    
    // 绘图相关
    QPoint m_origin; // 坐标原点
    double m_Ratio;   // 屏幕分辨率比例
    double m_scale;  // 缩放比例 (像素/mm)
    double m_markSpacing; // Mark中心间距，单位mm
    double m_ScreenWidth;
    
    void setupUI();
    void setupValidators();
    void setupConnections();
    void updateOriginAndScale();
    void drawCoordinateSystem(QPainter &painter);
    void drawPlatform(QPainter &painter, const Platform &platform, QColor color);
    void drawMark1(QPainter &painter);
    void drawMark2(QPainter &painter);
    void drawVirtualMark(QPainter &painter);
    QPointF transformPoint(const QPointF &point);
};

#endif // SIMULATIONPLATFORM_H