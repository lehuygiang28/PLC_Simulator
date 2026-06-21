/*
 * PLC Simulator - Industrial Communication Protocol Testing Tool
 * Copyright (c) 2025-2026 Wang Mao
 *
 * This file is part of PLC Simulator.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#include "CollapsibleGroupBox.h"

#include <QMouseEvent>
#include <QEvent>
#include <QFontMetrics>

CollapsibleGroupBox::CollapsibleGroupBox(QWidget* parent)
    : QGroupBox(parent)
{
    // 延迟到首次显示时记录原始标题并加箭头前缀
}

bool CollapsibleGroupBox::event(QEvent* e)
{
    // 在 polish(样式/标题就绪)后初始化标题前缀
    if (e->type() == QEvent::Polish && !m_titleCaptured)
    {
        m_baseTitle = title();
        m_titleCaptured = true;
        refreshTitlePrefix();
    }
    return QGroupBox::event(e);
}

int CollapsibleGroupBox::headerHeight() const
{
    // 标题行高度:字体高度 + 上下内边距余量
    return fontMetrics().height() + 18;
}

void CollapsibleGroupBox::refreshTitlePrefix()
{
    const QString arrow = m_collapsed ? QStringLiteral("▸ ")   // ▸
                                      : QStringLiteral("▾ ");   // ▾
    QGroupBox::setTitle(arrow + m_baseTitle);
}

void CollapsibleGroupBox::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed)
    {
        return;
    }
    m_collapsed = collapsed;

    const QList<QWidget*> kids = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* w : kids)
    {
        w->setVisible(!collapsed);
    }

    if (collapsed)
    {
        setMaximumHeight(headerHeight());
    }
    else
    {
        setMaximumHeight(QWIDGETSIZE_MAX);
    }
    refreshTitlePrefix();
}

void CollapsibleGroupBox::mousePressEvent(QMouseEvent* event)
{
    // 仅当点击落在标题行区域时切换折叠
    if (event->button() == Qt::LeftButton && event->position().toPoint().y() <= headerHeight())
    {
        setCollapsed(!m_collapsed);
        event->accept();
        return;
    }
    QGroupBox::mousePressEvent(event);
}
