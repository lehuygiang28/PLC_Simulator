/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifndef PLATFORMCANVAS_H
#define PLATFORMCANVAS_H

#include <QWidget>
#include <QPainter>
#include <QPoint>
#include "Core/PlatformTypes.h"

class PlatformScene;

/**
 * @brief 模拟平台画布:渲染 PlatformScene(坐标系/平台/Mark)
 *
 * 自治视图，持有 PlatformScene* 读取场景数据，自管 paintEvent、m_originPx。
 * 不依赖父类 SimulationPlatform 的任何绘制方法。
 */
class PlatformCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit PlatformCanvas(PlatformScene* scene, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateOrigin();
    void drawCoordinateSystem(QPainter& painter);
    void drawPlatform(QPainter& painter, const PlatformItem& item, QColor color, double radiusMm);
    void drawBaseMark(QPainter& painter);
    void drawLiveMark(QPainter& painter);
    void drawVirtualMark(QPainter& painter);
    QPointF transformPoint(const QPointF& pointMm) const;

    PlatformScene* m_scene;
    QPoint m_originPx;
};

#endif // PLATFORMCANVAS_H
