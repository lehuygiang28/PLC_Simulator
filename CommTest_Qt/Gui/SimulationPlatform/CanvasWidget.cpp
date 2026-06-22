#include "CanvasWidget.h"
#include "SimulationPlatform.h"
#include "ThemeManager.h"

CanvasWidget::CanvasWidget(SimulationPlatform* parent)
    : QWidget(parent)
    , m_parent(parent)
{
    // 背景与边框改由 paintEvent 按主题绘制(随主题实时变化)
}

void CanvasWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    ThemeManager& tm = ThemeManager::instance();

    // 画布背景跟随主题
    painter.fillRect(rect(), tm.color("@input"));

    // 调用父类的绘制方法
    if (m_parent) {
        m_parent->drawCanvas(painter);
    }

    // 主题色 1px 边框
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(tm.color("@border"));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    QWidget::paintEvent(event);
}
