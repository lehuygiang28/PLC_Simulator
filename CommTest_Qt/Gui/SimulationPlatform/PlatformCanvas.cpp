/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "PlatformCanvas.h"
#include "PlatformScene.h"
#include "ThemeManager.h"

// 绘图语义色:浅色主题沿用原色,深色主题用同色系提亮变体(在深底上仍可读);
// 坐标轴/刻度/文字跟随主题次级文字色。随主题实时变化(由 themeChanged 触发重绘)。
namespace {
inline bool platformDarkTheme() { return ThemeManager::instance().currentTheme() == Theme::Dark; }
inline QColor colCoordinateAxis()   { return ThemeManager::instance().color("@text2"); }
inline QColor colBasePlatform()     { return platformDarkTheme() ? QColor(0x5b, 0x9b, 0xd5) : QColor(Qt::blue); }
inline QColor colRealTimePlatform() { return platformDarkTheme() ? QColor(0x5c, 0xb8, 0x5c) : QColor(Qt::darkGreen); }
inline QColor colMark1()            { return platformDarkTheme() ? QColor(0xff, 0x6b, 0x6b) : QColor(Qt::red); }
inline QColor colMark2()            { return platformDarkTheme() ? QColor(0xd0, 0x70, 0xd0) : QColor(Qt::darkMagenta); }
inline QColor colVirtualMark()      { return platformDarkTheme() ? QColor(0x3c, 0xc7, 0xc7) : QColor(Qt::darkCyan); }
}

PlatformCanvas::PlatformCanvas(PlatformScene* scene, QWidget* parent)
    : QWidget(parent)
    , m_scene(scene)
{
    // scene 变化 → 画布重绘(自连)
    connect(m_scene, &PlatformScene::changed, this, [this]{ update(); });
}

void PlatformCanvas::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    ThemeManager& tm = ThemeManager::instance();

    // 画布背景跟随主题
    painter.fillRect(rect(), tm.color("@input"));

    // 更新原点
    updateOrigin();

    // 绘制坐标系
    drawCoordinateSystem(painter);

    // 绘制基准平台
    if (m_scene->platform(Platform::Base).visible)
        drawPlatform(painter, m_scene->platform(Platform::Base), colBasePlatform(), 20.0);

    // 绘制实时平台
    if (m_scene->platform(Platform::Live).visible)
        drawPlatform(painter, m_scene->platform(Platform::Live), colRealTimePlatform(), 15.0);

    // 绘制Mark1
    if (m_scene->baseMark().visible)    drawBaseMark(painter);

    // 绘制Mark2
    if (m_scene->liveMark().visible)    drawLiveMark(painter);

    // 绘制VirtualMark
    if (m_scene->virtualMark().visible) drawVirtualMark(painter);

    // 主题色 1px 边框
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(tm.color("@border"));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    QWidget::paintEvent(event);
}

void PlatformCanvas::resizeEvent(QResizeEvent* event)
{
    updateOrigin();
    QWidget::resizeEvent(event);
    update();
}

void PlatformCanvas::updateOrigin()
{
    // 设置坐标原点为中心点
    m_originPx.setX(width() / 2);
    m_originPx.setY(height() / 2);
}

void PlatformCanvas::drawCoordinateSystem(QPainter& painter)
{
    painter.save();

    QPen pen(colCoordinateAxis(), 1, Qt::SolidLine);
    painter.setPen(pen);

    // 绘制X轴和Y轴
    painter.drawLine(0, m_originPx.y(), width(), m_originPx.y());  // X轴
    painter.drawLine(m_originPx.x(), 0, m_originPx.x(), height()); // Y轴

    // 绘制网格线和刻度标签
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    const double scale = m_scene->pixelsPerMm();

    // X轴正方向刻度
    for (int i = 0; i * scale < width() - m_originPx.x(); i += 10)
    {
        int x = m_originPx.x() + i * scale;
        painter.drawLine(x, m_originPx.y() - 3, x, m_originPx.y() + 3);
        // 间隔一个循环显示标签
        if ((i / 10) % 2 == 0 || i == 0)
            painter.drawText(x - 10, m_originPx.y() + 15, QString::number(i));
    }

    // X轴负方向刻度
    for (int i = 0; m_originPx.x() - i * scale > 0; i += 10)
    {
        int x = m_originPx.x() - i * scale;
        painter.drawLine(x, m_originPx.y() - 3, x, m_originPx.y() + 3);
        if ((i / 10) % 2 == 0 && i != 0)
            painter.drawText(x - 10, m_originPx.y() + 15, QString::number(-i));
    }

    // Y轴正方向刻度
    for (int i = 0; i * scale < m_originPx.y(); i += 10)
    {
        int y = m_originPx.y() - i * scale;
        painter.drawLine(m_originPx.x() - 3, y, m_originPx.x() + 3, y);
        if ((i / 10) % 2 == 0 && i != 0)
            painter.drawText(m_originPx.x() + 5, y + 5, QString::number(i));
    }

    // Y轴负方向刻度
    for (int i = 0; m_originPx.y() + i * scale < height(); i += 10)
    {
        int y = m_originPx.y() + i * scale;
        painter.drawLine(m_originPx.x() - 3, y, m_originPx.x() + 3, y);
        if ((i / 10) % 2 == 0 && i != 0)
            painter.drawText(m_originPx.x() + 5, y + 5, QString::number(-i));
    }

    painter.restore();
}

void PlatformCanvas::drawPlatform(QPainter& painter, const PlatformItem& item, QColor color, double radiusMm)
{
    painter.save();

    const double scale = m_scene->pixelsPerMm();

    // 计算平台在屏幕上的位置
    QPointF center = transformPoint(QPointF(item.pose.x, item.pose.y));

    // 设置画笔和画刷
    QPen pen(color, 2);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // 绘制圆形平台
    double radius = radiusMm * scale;

    painter.drawEllipse(center, radius, radius);

    // 保存当前变换矩阵
    painter.save();

    // 移动到平台中心并旋转
    painter.translate(center.x(), center.y());
    painter.rotate(item.pose.angleDeg);

    // 绘制表示方向的十字线 (长度30mm)
    double lineLength = 30 * scale;
    painter.drawLine(-lineLength, 0, lineLength, 0); // X轴方向线
    painter.drawLine(0, -lineLength, 0, lineLength); // Y轴方向线

    // 恢复变换矩阵
    painter.restore();
}

void PlatformCanvas::drawBaseMark(QPainter& painter)
{
    painter.save();

    const double scale = m_scene->pixelsPerMm();
    const Pose finalPose = m_scene->baseMarkFinalPose();

    QPointF pos = transformPoint(QPointF(finalPose.x, finalPose.y));

    // 设置画笔
    QPen pen(colMark1(), 1);
    painter.setPen(pen);
    painter.setBrush(colMark1());

    // 保存当前变换
    painter.save();

    // 移动到Mark位置并旋转
    painter.translate(pos.x(), pos.y());
    painter.rotate(finalPose.angleDeg);

    // 绘制L型
    double spacing = m_scene->markCenterDistance() / 2 * scale;   // 中心间距
    double rectWidth = MARK_RECT_WIDTH * scale;   // 矩形宽度
    double rectHeight = MARK_RECT_HEIGHT * scale; // 矩形高度

    // 绘制左边的L型mark
    // 水平部分（横杠）- 左边
    painter.drawRect(-spacing - rectHeight, -rectWidth, rectHeight, rectWidth);
    // 垂直部分（竖杠）- 左边
    painter.drawRect(-spacing - rectWidth, -rectHeight, rectWidth, rectHeight);

    // 绘制右边的L型mark
    // 水平部分（横杠）- 右边
    painter.drawRect(spacing, -rectWidth, rectHeight, rectWidth);
    // 垂直部分（竖杠）- 右边
    painter.drawRect(spacing, -rectHeight, rectWidth, rectHeight);

    // 恢复变换
    painter.restore();

    // painter.restore();  // 原注释掉的标签绘制区域
}

void PlatformCanvas::drawLiveMark(QPainter& painter)
{
    painter.save();

    const double scale = m_scene->pixelsPerMm();
    const Pose finalPose = m_scene->liveMarkFinalPose();

    QPointF pos = transformPoint(QPointF(finalPose.x, finalPose.y));

    // 设置画笔
    QPen pen(colMark2(), 1);
    painter.setPen(pen);
    painter.setBrush(colMark2());

    // 保存当前变换
    painter.save();

    // 移动到Mark位置并旋转
    painter.translate(pos.x(), pos.y());
    painter.rotate(finalPose.angleDeg);

    // 绘制L型
    double spacing = m_scene->markCenterDistance() / 2 * scale;   // 中心间距
    double rectWidth = MARK_RECT_WIDTH * scale;   // 矩形宽度
    double rectHeight = MARK_RECT_HEIGHT * scale; // 矩形高度

    // 绘制水平部分
    painter.drawRect(-spacing, 0, rectHeight, rectWidth);
    // 绘制垂直部分
    painter.drawRect(-spacing, 0, rectWidth, rectHeight);

    // 绘制水平部分
    painter.drawRect(spacing - rectHeight, 0, rectHeight, rectWidth);
    // 绘制垂直部分
    painter.drawRect(spacing - rectWidth, 0, rectWidth, rectHeight);

    // 恢复变换
    painter.restore();

    // painter.restore();  // 原注释掉的标签绘制区域
}

void PlatformCanvas::drawVirtualMark(QPainter& painter)
{
    painter.save();

    const double scale = m_scene->pixelsPerMm();

    // 1. 计算Mark2的最终位置和角度(用 scene 的 liveMarkFinalPose)
    const Pose liveMarkFinal = m_scene->liveMarkFinalPose();

    QPointF pos = transformPoint(QPointF(liveMarkFinal.x, liveMarkFinal.y));

    // 设置画笔
    QPen pen(colVirtualMark(), 2);
    painter.setPen(pen);

    // 移动到Mark2中心位置并旋转
    painter.translate(pos.x(), pos.y());
    painter.rotate(liveMarkFinal.angleDeg);

    // 计算参数
    double spacing = m_scene->markCenterDistance() / 2 * scale;   // Mark2左右侧的中心间距
    // 最终偏移量 = 默认偏移量 + 用户设定值
    double offsetX = (VIRTUAL_MARK_STATIC_X + m_scene->virtualMark().pose.x) * scale;  // X偏移（向外为正）
    double offsetY = (VIRTUAL_MARK_STATIC_Y + m_scene->virtualMark().pose.y) * scale;  // Y偏移（向上为正）
    double rectWidth = MARK_RECT_WIDTH * scale;   // 矩形宽度
    double rectHeight = MARK_RECT_HEIGHT * scale; // 矩形高度

    painter.setBrush(colVirtualMark());

    // 绘制左侧十字Mark（相对左侧Mark2向外、向上偏移）
    // 左侧Mark2中心在 (-spacing, 0)，向外偏移即X减小，向上偏移即Y减小（屏幕坐标）
    double leftCenterX = -spacing - offsetX;
    double leftCenterY = -offsetY;
    // 水平矩形（宽为rectHeight，高为rectWidth，居中）
    painter.drawRect(QRectF(leftCenterX - rectHeight / 2, leftCenterY - rectWidth / 2,
                            rectHeight, rectWidth));
    // 垂直矩形（宽为rectWidth，高为rectHeight，居中）
    painter.drawRect(QRectF(leftCenterX - rectWidth / 2, leftCenterY - rectHeight / 2,
                            rectWidth, rectHeight));

    // 绘制右侧十字Mark（相对右侧Mark2向外、向上偏移）
    // 右侧Mark2中心在 (spacing, 0)，向外偏移即X增大，向上偏移即Y减小（屏幕坐标）
    double rightCenterX = spacing + offsetX;
    double rightCenterY = -offsetY;
    // 水平矩形
    painter.drawRect(QRectF(rightCenterX - rectHeight / 2, rightCenterY - rectWidth / 2,
                            rectHeight, rectWidth));
    // 垂直矩形
    painter.drawRect(QRectF(rightCenterX - rectWidth / 2, rightCenterY - rectHeight / 2,
                            rectWidth, rectHeight));

    painter.restore();
}

QPointF PlatformCanvas::transformPoint(const QPointF& pointMm) const
{
    // 将世界坐标(mm)转换为屏幕坐标(pixel)
    return QPointF(m_originPx.x() + pointMm.x() * m_scene->pixelsPerMm(),
                   m_originPx.y() - pointMm.y() * m_scene->pixelsPerMm()); // 注意Y轴翻转
}
