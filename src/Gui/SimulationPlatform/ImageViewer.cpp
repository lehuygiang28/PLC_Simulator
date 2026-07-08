/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "ImageViewer.h"
#include "ThemeManager.h"
#include <QResizeEvent>
#include <QShowEvent>

ImageViewer::ImageViewer(QWidget* parent)
    : QWidget(parent)
    , m_scale(1.0)
    , m_minScale(0.1)
    , m_maxScale(10.0)
    , m_offset(0, 0)
    , m_dragging(false)
    , m_fitMode(true)
    , m_pendingFit(false)
{
    setMinimumSize(200, 200);
    setMouseTracking(true);
}

void ImageViewer::setImage(const QImage& image)
{
    m_image = image;
    m_offset = QPointF(0, 0);
    // 自动计算缩放比例使图像完整显示
    fitToWindow();
}

void ImageViewer::fitToWindow()
{
    if (m_image.isNull())
        return;

    // 尺寸尚未就绪(控件未布局/未显示):标记延迟,待首次有效尺寸再 fit
    if (width() <= 0 || height() <= 0) {
        m_pendingFit = true;
        return;
    }

    // 计算使图像完整显示所需的缩放比例(高或宽与画布相当)
    double scaleX = static_cast<double>(width()) / m_image.width();
    double scaleY = static_cast<double>(height()) / m_image.height();
    m_scale = qBound(m_minScale, qMin(scaleX, scaleY), m_maxScale);

    m_offset = QPointF(0, 0);
    m_fitMode = true;      // fit 即进入适应模式
    m_pendingFit = false;
    emit scaleChanged(m_scale);
    update();
}

bool ImageViewer::hasImage() const
{
    return !m_image.isNull();
}

const QImage& ImageViewer::image() const
{
    return m_image;
}

double ImageViewer::scale() const
{
    return m_scale;
}

void ImageViewer::setScale(double scale)
{
    double newScale = qBound(m_minScale, scale, m_maxScale);
    if (qFuzzyCompare(newScale, m_scale)) return;
    m_scale = newScale;
    m_fitMode = false;   // 显式设定缩放 → 自由模式
    update();
    emit scaleChanged(m_scale);
}

void ImageViewer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    // 根据缩放比例选择合适的插值算法
    // 放大时（scale > 1.0）：使用最近邻插值，保持清晰锐利
    // 缩小时（scale < 1.0）：使用双线性插值，避免锯齿
    if (m_scale < 1.0) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    } else {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    }

    // 绘制灰白棋盘格背景
    drawCheckerboard(painter);

    if (m_image.isNull()) {
        painter.setPen(ThemeManager::instance().color("@text2"));
        painter.drawText(rect(), Qt::AlignCenter, "No Image Loaded");
        return;
    }

    // 计算缩放后的图像尺寸
    QSizeF scaledSize = m_image.size() * m_scale;

    // 计算绘制位置（居中 + 偏移）
    QPointF center(width() / 2.0, height() / 2.0);
    QPointF topLeft = center - QPointF(scaledSize.width() / 2.0, scaledSize.height() / 2.0) + m_offset;

    // 绘制图像
    painter.drawImage(QRectF(topLeft, scaledSize), m_image);
}

void ImageViewer::drawCheckerboard(QPainter& painter)
{
    const int gridSize = 16;  // 棋盘格单元大小
    const bool dark = ThemeManager::instance().currentTheme() == Theme::Dark;
    // 两档交替色:浅色=白/浅蓝白;深色=两档深灰
    const QColor cellA = dark ? QColor(0x3a, 0x3d, 0x41) : QColor(0xff, 0xff, 0xff);
    const QColor cellB = dark ? QColor(0x2b, 0x2d, 0x30) : QColor(0xdb, 0xe9, 0xf7);

    for (int y = 0; y < height(); y += gridSize) {
        for (int x = 0; x < width(); x += gridSize) {
            bool isCellA = ((x / gridSize) + (y / gridSize)) % 2 == 0;
            painter.fillRect(x, y, gridSize, gridSize, isCellA ? cellA : cellB);
        }
    }
}

void ImageViewer::wheelEvent(QWheelEvent* event)
{
    if (m_image.isNull()) return;

    // 获取鼠标位置（相对于控件）
    QPointF mousePos = event->position();

    // 计算鼠标位置相对于图像中心的偏移（缩放前）
    QPointF center(width() / 2.0, height() / 2.0);
    QPointF relativePos = mousePos - center - m_offset;

    // 计算新的缩放比例
    double oldScale = m_scale;
    double delta = event->angleDelta().y() / 120.0;
    double factor = 1.0 + delta * 0.1;
    m_scale *= factor;
    m_scale = qBound(m_minScale, m_scale, m_maxScale);

    // 调整偏移以保持鼠标位置不变
    double scaleRatio = m_scale / oldScale;
    m_offset = mousePos - center - relativePos * scaleRatio;

    m_fitMode = false;   // 用户手动缩放 → 转自由模式,resize 不再强制铺满
    update();
    emit scaleChanged(m_scale);
    event->accept();
}

void ImageViewer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
        m_dragging = true;
        setCursor(Qt::ClosedHandCursor);
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        QPointF delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        m_fitMode = false;   // 用户拖动平移 → 转自由模式
        update();
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void ImageViewer::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    // 双击恢复图像完整显示
    fitToWindow();
}

void ImageViewer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (!m_image.isNull() && width() > 0 && height() > 0) {
        // 待适应(首次就绪)或适应模式:重新按当前尺寸铺满
        // 自由模式(用户已缩放/平移):保持绝对缩放与偏移,仅由 paintEvent 按新尺寸重新居中
        if (m_pendingFit || m_fitMode)
            fitToWindow();
    }
    update();
}

void ImageViewer::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    // 加载时尺寸未就绪而延迟的 fit,在控件首次显示(已是真实尺寸)时补做
    if (m_pendingFit && !m_image.isNull() && width() > 0 && height() > 0)
        fitToWindow();
}